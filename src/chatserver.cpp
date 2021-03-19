#include "socket.h"
#include "chatserver.h"

//should be called getLowercase()
std::string getLowercase(const std::string& text) {
    std::string lowered = text;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return lowered;
}


void Room::addUser(User && user) {
    //user_sockets.insert(user.socket);
    if (user.has_name && !silent) {
        std::string join_message = "New user joined: " + user.username + "\n";
        sendMessageToOthers(join_message.c_str(), join_message.size() + 1, user);
    }
    users.push_back(std::move(user));
    user_count++;
}

void Room::removeUser(const User& user) {
    removal_sockets.insert(user.socket);
    user_count--;
}

void Room::processRemoval() {
    for (auto && socket : removal_sockets) {
        for (auto it = users.begin(); it != users.end(); it++) {
            if (it->socket == socket) {
                //std::cout << "erasing..." << std::endl;
                users.erase(it);
                //std::cout << "erased!" << std::endl;
                break;
            }
        }
    }
}


//does not care about fails, does not send to sender
void Room::sendMessageToOthers(const char* buffer, size_t buffer_size, const User & sender) {
    for (auto && user : users) {
        if (user.socket != sender.socket) {
            Socket::sendMessage(user.socket, buffer, buffer_size);
        }
    }
}

void Room::checkAndProcessMessages() {
    char buffer[1024] = {0};
    size_t size = 0;


    for (auto && user : users) {
        if (!Socket::actionOccured(user.socket)) {
            continue;
        }

        //std::cout << "Waiting for message..." << std::endl;
        if ((size = Socket::getMessage(user.socket, buffer, sizeof(buffer))) <= 0) {
            continue;
        }
        //std::cout << "Got Message!" << std::endl;

        std::string message(buffer);
        sanitizeMessage(message);

        processMessage(std::move(message), user);
    }

    processRemoval();
    //std::cout << "room: " << this->name << " completed its iteration" << std::endl;
}

void Room::processMessage(std::string && message, User & user) {
    if (!user.has_name) {
        if (validateName(message)) {
            user.username = message;
            user.has_name = true;
            if (!silent) {
                std::string join_message = "New user joined: " + user.username + "\n";
                sendMessageToOthers(join_message.c_str(), join_message.size() + 1, user);    
            }
        }
        else {
            const char error[] = "Please enter a valid name (only use ASCII digits/letters).";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        return;
    }

    if (message == "/rooms") {
        for (auto it = room_handler->rooms.begin(); it != room_handler->rooms.end(); it++) {
            std::string room_name = it->second.name + " (" + std::to_string(it->second.user_count) + ")\n";
            if (it->second.name == this->name) {
                room_name = "->" + room_name;
            }
            else {
                room_name = "  " + room_name;
            }
            Socket::sendMessage(user.socket, room_name.c_str(), room_name.size() + 1);
        }
        return;
    }

    if (message == "/users") {
        for (auto it = users.begin(); it != users.end(); it++) {
            
            std::string username = it->username;
            if (it->socket == user.socket) {
                username += " (you)";
            }
            username += "\n";
            Socket::sendMessage(user.socket, username.c_str(), username.size() + 1);
        }
        return;
    }

    ///check if name exists
    if (message.rfind("/create ", 0) == 0) {

        std::string new_room_name = message.substr(8);
        sanitizeMessage(new_room_name);
        if (!validateName(new_room_name)) {
            return;
        }

        if (room_handler->roomNameTaken(new_room_name)) {
            const char error[] = "This room already exists.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }

        room_handler->addNewRoomEntry(new_room_name);
        //Room new_room(new_room_name, room_handler);
        //room_handler->new_rooms.push(new_room);
        return;
    }

    //TODO: finish
    if (message.rfind("/remove ", 0) == 0) {

        std::string removed_room_name = message.substr(8);
        sanitizeMessage(removed_room_name);
        if (!validateName(removed_room_name)) {
            return;
        }

        
        return;
    }


    if (message.rfind("/join ", 0) == 0) {
        std::string new_room_name = message.substr(6);
        sanitizeMessage(new_room_name);

        if (getLowercase(new_room_name) == getLowercase(this->name)) {
            const char error[] = "You are already in this room.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
            return;
        }

        if (!room_handler->roomExists(new_room_name)) {
            const char error[] = "The room specified does not exist.\nYou can create rooms using the /create command\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
            return;
        }

        auto target_room = room_handler->findRoom(new_room_name);
        room_handler->moveUser(user, *this, target_room->second);
        return;
    }


    if (message.rfind("/w ", 0) == 0) {
        std::string payload = message.substr(3);
        sanitizeMessage(payload);
        size_t sep_index = payload.find_first_of(" \t\n\r");
        if (sep_index == std::string::npos) {
            const char error[] = "Wrong /w usage. /w format: /w <user> <message>\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
            return;
        }
        std::string target_name = payload.substr(0, sep_index);

        auto target_user = users.begin();
        for (; target_user != users.end(); target_user++) {
            if (target_user->username == target_name) {
                break;
            }
        }
        if (target_user == users.end()) {
            const char error[] = "User not found. Use /users to list all users in this room\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
            return;
        }


        std::string content = payload.substr(sep_index);
        sanitizeMessage(content);
        content = "(w)" + user.username + ": " + content + "\n";
        Socket::sendMessage(target_user->socket, content.c_str(), content.size() + 1);
        return;
    }

    

    if (silent) {
        const char error[] = "This room is silent, your messages will not be shown.\n";
        Socket::sendMessage(user.socket, error, sizeof(error));
        return;
    }    


    std::string named_message = user.username + ": " + message + "\n";
    sendMessageToOthers(named_message.c_str(), named_message.length() + 1, user);

}



bool Room::validateName(const std::string & name) {
    for (auto it = name.cbegin(); it != name.cend(); it++) {
        const char c = *it;
        if ((c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
            continue;
        }
        else {
            return false;
        }
    }
    return true;
}

void Room::sanitizeMessage(std::string& message) {
    message.erase(0, message.find_first_not_of(" \t\n\r"));
    message.erase(message.find_last_not_of(" \t\n\r") + 1);
}




//////////////////////////////////////////////////


bool RoomHandler::roomExists(const std::string& room_name) {
    std::string lowercase_name = getLowercase(room_name);
    if (rooms.find(lowercase_name) != rooms.end()) {
        return true;
    }
    return false;
}


bool RoomHandler::roomNameTaken(const std::string& room_name) {
    std::string lowercase_name = getLowercase(room_name);
    if (room_names.find(lowercase_name) != room_names.end()) {
        return true;
    }
    return false;
}

void RoomHandler::addNewRoomEntry(const std::string& room_name) {
    Room new_room(room_name, this);
    new_rooms.push(new_room);
    room_names.insert(getLowercase(room_name));
}

void RoomHandler::processNewRoomEntries() {
    while (!new_rooms.empty()) {
        Room room = new_rooms.top(); //TODO: move, dont copy
        new_rooms.pop();
        rooms.insert({getLowercase(room.name), room});
    }
}

std::map<std::string, Room>::iterator RoomHandler::findRoom(const std::string& room_name) {
    std::string lowercase_name = getLowercase(room_name);
    return rooms.find(lowercase_name);
}

void RoomHandler::moveUser(User& user, Room& source, Room& target) {
    User user_cpy = user;
    target.addUser(std::move(user_cpy));
    source.removeUser(user);
}



////////////////////////////////////////////////////






void ChatServer::mainLoop() {

    while (true) {
        int activities = Socket::checkActivity(sockets);
        if (activities <= 0) {
            if (activities == -1) {
                std::cerr << "error: select() error" << std::endl;
            }
            continue;
            //return;
        }

        //std::cout << "Action occured" << std::endl;

        if (Socket::actionOccured(server_socket)) {
            acceptConnection();
            std::cout << "New user Connected" << std::endl;
        }

        //add new rooms
        room_handler.processNewRoomEntries();    

        for (auto it = room_handler.rooms.begin(); it != room_handler.rooms.end(); it++) {
            it->second.checkAndProcessMessages();
        }
    }
}

//TODO: send instructions
void ChatServer::acceptConnection() {
    int new_socket;
    if ((new_socket = Socket::getNewClientSocket(server_socket)) == -1) {
        return;
    }

    //std::cout << "test" << std::endl;

    sockets.insert(new_socket);
    User new_user(new_socket);
    //users[new_socket] = new_user;

    //find silent room
    //rooms["Silent"].addUser(std::move(new_user));
    auto silent = room_handler.rooms.find("silent");
    if (silent != room_handler.rooms.end()) {
        silent->second.addUser(std::move(new_user));
    }
    else {
        std::cerr << "Silent Room does not exist" << std::endl;
    }

    const char instructions[] = "Welcome to ChatServer\nEnter your name:\n";

    Socket::sendMessage(new_socket, instructions, sizeof(instructions));
}

int ChatServer::createServerSocket(int port) {
    server_socket = Socket::createServerSocket(port);
    if (server_socket == -1) {
        return -1;
    }
    sockets.insert(server_socket);
    return server_socket;
}




