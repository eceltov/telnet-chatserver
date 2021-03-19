#include "socket.h"
#include "chatserver.h"




void Room::addUser(User && user) {
    //user_sockets.insert(user.socket);
    if (user.has_name && !silent) {
        std::string join_message = "New user joined: " + user.username + "\n";
        sendMessageToOthers(join_message.c_str(), join_message.size() + 1, user);
    }
    users.push_back(std::move(user));
    user_count++;
}

void Room::removeUser(int socket) {
    removal_sockets.insert(socket);
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
        for (auto it = rooms->begin(); it != rooms->end(); it++) {
            std::string room_name = it->first + " (" + std::to_string(it->second.user_count) + ")\n";
            if (it->first == this->name) {
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

        Room new_room(new_room_name, rooms, new_rooms);
        //rooms->insert({new_room_name, new_room});
        new_rooms->push(new_room);
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

        if (rooms->find(new_room_name) == rooms->end()) {
            const char error[] = "The room specified does not exist.\nYou can create rooms using the /create command\n";
            //user.creating_room = true;
            //user.created_room_name = new_room_name;
            Socket::sendMessage(user.socket, error, sizeof(error));
            return;
        }

        moveUser(user, new_room_name);
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

    

    if (name == "Silent") {
        const char error[] = "This room is silent, your messages will not be shown.\n";
        Socket::sendMessage(user.socket, error, sizeof(error));
        return;
    }    


    std::string named_message = user.username + ": " + message + "\n";
    sendMessageToOthers(named_message.c_str(), named_message.length() + 1, user);

}

void Room::moveUser(User & user, std::string & new_room_name) {
    User user_cpy = user;
    //(*rooms)[new_room_name].addUser(std::move(user_cpy));
    auto new_room = rooms->find(new_room_name);
    if (new_room == rooms->end()) {
        std::cerr << "Move failure, room does not exist" << std::endl;
    }
    new_room->second.addUser(std::move(user_cpy));
    removeUser(user.socket);
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
        while (!new_rooms.empty()) {
            Room room = new_rooms.top(); //TODO: move, dont copy
            new_rooms.pop();
            rooms.insert({room.name, room});
        }

        for (auto it = rooms.begin(); it != rooms.end(); it++) {
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
    auto silent = rooms.find("Silent");
    if (silent != rooms.end()) {
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




