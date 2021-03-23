#include "socket.h"
#include "chatserver.h"

std::string getLowercase(const std::string& text) {
    std::string lowered = text;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return lowered;
}

//checks whether @name contains only letters and numbers
bool validateName(const std::string& name) {
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

//removes leading and trailing whitespace
void sanitizeMessage(std::string& message) {
    message.erase(0, message.find_first_not_of(" \t\n\r"));
    message.erase(message.find_last_not_of(" \t\n\r") + 1);
}

//////////////////
///RoomHandler////
//////////////////

RoomHandler::RoomHandler(std::string& default_room_name_) {
    default_room_name = default_room_name_;
    Room default_room(default_room_name);
    default_room.silent = true;
    default_room.removable = false;
    rooms.insert({getLowercase(default_room_name), default_room});
  }


//checks whether a room with the name @room_name exists
//rooms which are to be added later (in @RoomHandler::new_rooms) are ignored
bool RoomHandler::roomExists(const std::string& room_name) {
    std::string lowercase_name = getLowercase(room_name);
    if (rooms.find(lowercase_name) != rooms.end()) {
        return true;
    }
    return false;
}

//checks whether @room_name is reserved (either by existing
//rooms or by rooms to be added in @RoomHandler::new_rooms)
bool RoomHandler::roomNameTaken(const std::string& room_name) {
    std::string lowercase_name = getLowercase(room_name);
    if (room_names.find(lowercase_name) != room_names.end()) {
        return true;
    }
    return false;
}

//creates a new room and marks its name in @RoomHandler::room_names
void RoomHandler::addNewRoomEntry(const std::string& room_name) {
    Room new_room(room_name);
    new_rooms.push(new_room);
    room_names.insert(getLowercase(room_name));
}

//adds rooms created by RoomHandler::addNewRoomEntry() to the main
//room container RoomHandler::rooms
//needs to be called before the next set of user actions is processed
void RoomHandler::processNewRoomEntries() {
    while (!new_rooms.empty()) {
        Room room = new_rooms.top();
        new_rooms.pop();
        rooms.insert({getLowercase(room.name), room});
    }
}

//returns an iterator pointing at the room specified by the name @room_name
std::map<std::string, Room>::iterator RoomHandler::findRoom(const std::string& room_name) {
    std::string lowercase_name = getLowercase(room_name);
    return rooms.find(lowercase_name);
}

//moves the user from @source room to the @target room
//sends an info message to the @target room announcing that a new user joined 
void RoomHandler::moveUser(User& user, Room& source, Room& target) {
    User user_cpy = user;
    if (!target.silent) {
        std::string join_message = "New user joined: " + user_cpy.username + "\n";
        sendMessageToRoom(join_message.c_str(), join_message.size() + 1, user_cpy, target);
    }
    target.users.push_back(std::move(user_cpy));
    target.user_count++;
    source.removal_sockets.insert(user.socket);
    source.user_count--;
}

//creates a new user from the given @user_socket and adds him to default room
void RoomHandler::createUser(int user_socket) {
    auto default_room = findRoom(default_room_name);

    User user(user_socket);
    default_room->second.users.push_back(std::move(user));
    default_room->second.user_count++;
}

//prepares @user from @source room to be removed
//removes @user.username from known usernames stored in RoomHandler::usernames
//actual removal is carried out by RoomHandler::processUserRemoval()
void RoomHandler::removeUser(User& user, Room& source) {
    usernames.erase(user.username);
    user.leaving = true;
    source.removal_sockets.insert(user.socket);
    source.user_count--;

}

//gives @user an unsername
//expects that @user does not have an username and that @username is valid
void RoomHandler::nameUser(User& user, const std::string& username) {
    user.username = username;
    user.has_name = true;
    usernames.insert(user.username);
}

//gives @user a different username
//updates known usernames stored in RoomHandler::usernames
//expects that @new_username is valid
void RoomHandler::renameUser(User& user, const std::string& new_username) {
    usernames.erase(user.username);
    user.username = new_username;
    usernames.insert(new_username);
}

//checks whether @username is taken by an existing user
bool RoomHandler::usernameExists(const std::string& username) {
    if (usernames.empty()) {
        return false;
    }
    return usernames.find(username) != usernames.end();
}

//return the amount of users present in room with name @room_name
size_t RoomHandler::getRoomPopulation(const std::string& room_name) {
    std::string lowercase_name = getLowercase(room_name);
    auto room = findRoom(lowercase_name);
    return room->second.user_count;
}

//removes room with name @room_name from RoomHandler::rooms
//removes the rooms name from known room names stored in RoomHandler::room_names
//expects that the specified room exists
//returns false if the room is unremovable
bool RoomHandler::removeRoom(const std::string& room_name) {
    std::string lowercase_name = getLowercase(room_name);
    auto room = findRoom(room_name);
    if (!room->second.removable) {
        return false;
    }
    rooms.erase(lowercase_name);
    room_names.erase(room_name);
    return true;
}

//return iterator pointing at a user with username @username
std::list<User>::const_iterator RoomHandler::findUser(const std::string& username, const Room& source) const {
    std::list<User>::const_iterator user_it = source.users.begin();
    for (; user_it != source.users.end(); user_it++) {
        if (user_it->username == username) {
            break;
        }
    }
    return user_it;
}

//removes users specified by their sockets stored in Room::removal_sockets
//if the user is flagged as "leaving", this will also close its socket and
//remove it from known sockets stored in RoomHandler::sockets 
void RoomHandler::processUserRemoval(Room& source) {
    for (auto && socket : source.removal_sockets) {
        for (auto it = source.users.begin(); it != source.users.end(); it++) {
            if (it->socket == socket) {
                bool leaving = it->leaving;
                source.users.erase(it);
                if (leaving) {
                    Socket::closeSocket(socket);
                }
                break;
            }
        }
    }
    source.removal_sockets.clear();
}

//main method which handles user actions
//to be invoked when a socket action occured
void RoomHandler::processActions(fd_set* fds) {
    //adds rooms created in previous invocation
    processNewRoomEntries();    

    for (auto && room : rooms) {
        char buffer[1024] = {0};
        size_t size = 0;

        for (auto && user : room.second.users) {
            if (!Socket::actionOccured(user.socket, fds)) {
                continue;
            }

            if ((size = Socket::getMessage(user.socket, buffer, sizeof(buffer))) <= 0) {
                continue;
            }

            std::string message(buffer);
            sanitizeMessage(message);
            processMessage(message, user, room.second);
        }

        if (!room.second.removal_sockets.empty()) {
            processUserRemoval(room.second);
        }
    }
}

//creates a response to the @message of @user in a given @room
void RoomHandler::processMessage(std::string& message, User& user, Room& room) {
    //do nothing if message is empty
    if (message.empty()) {
        return;
    }
    if (message[0] != '/') {
        //gives user an username if absent
        if (!user.has_name) {
            if (!validateName(message)) {
                const char error[] = "Please enter a valid name (only use ASCII digits/letters).\n";
                Socket::sendMessage(user.socket, error, sizeof(error));
            }
            else if (usernameExists(message)) {
                const char error[] = "This name is taken, try a different name.\n";
                Socket::sendMessage(user.socket, error, sizeof(error));
            }
            else {
                nameUser(user, message);
                const char info[] = "Welcome to the server. Type /help for a summary of commands.\n";
                Socket::sendMessage(user.socket, info, sizeof(info));
                if (!room.silent) {
                    std::string join_message = "New user joined: " + user.username + "\n";
                    sendMessageToRoom(join_message.c_str(), join_message.size() + 1, user, room);    
                }
            }
        }
        //displays an error message if the user attempts to send messages in a silent room
        else if (room.silent) {
            const char error[] = "This room is silent, your messages will not be shown.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        //sends a message to all users in the room
        else {
            std::string named_message = user.username + ": " + message + "\n";
            sendMessageToRoom(named_message.c_str(), named_message.length() + 1, user, room);
        }
        return;
    }
    //do nothing if the message is a single '/'
    if (message.size() == 1) {
        return;
    }

    //split string into command and content that follows
    size_t cmd_start =  message.find_first_not_of(" \t\n\r", 1);
    size_t cmd_end = message.find_first_of(" \t\n\r", cmd_start);
    std::string command = message.substr(cmd_start, cmd_end - cmd_start);
    std::string content;
    if (cmd_end != std::string::npos) {
        size_t con_start = message.find_first_not_of(" \t\n\r", cmd_end);
        content = message.substr(con_start);
    } 

    //removes the user from server
    if (command == "leave") {
        const char info[] = "Goodbye.\n";
        Socket::sendMessage(user.socket, info, sizeof(info));
        if (!room.silent) {
            const char announcement[] = "A user has left the room.\n";
            sendMessageToRoom(announcement, sizeof(announcement), user, room);
        }
        removeUser(user, room);
    }
    //unnamed users cannot issue other commands other than leave
    else if (!user.has_name) {
        const char error[] = "Please enter a name first (or if you want to leave instead, use /leave).\n";
        Socket::sendMessage(user.socket, error, sizeof(error));
        return;
    }
    //displays a list of existing rooms with user counts
    else if (command == "rooms") {
        for (auto it = rooms.begin(); it != rooms.end(); it++) {
            std::string room_name = it->second.name + " (" + std::to_string(it->second.user_count) + ")\n";
            if (it->second.name == room.name) {
                room_name = "->" + room_name;
            }
            else {
                room_name = "  " + room_name;
            }
            Socket::sendMessage(user.socket, room_name.c_str(), room_name.size() + 1);
        }
    }
    //displays a list of existing users in current room
    else if (command == "users") {
        for (auto it = room.users.begin(); it != room.users.end(); it++) {
            std::string username = it->username;
            if (it->socket == user.socket) {
                username += " (you)";
            }
            username += "\n";
            Socket::sendMessage(user.socket, username.c_str(), username.size() + 1);
        }
    }
    //creates a new room
    else if (command == "create" && !command.empty()) {
        std::string new_room_name = std::move(content);
        if (!validateName(new_room_name)) {
            const char error[] = "This name is not valid.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else if (roomNameTaken(new_room_name)) {
            const char error[] = "This room already exists.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else {
            addNewRoomEntry(new_room_name);
            const char info[] = "Room created successfully.\n";
            Socket::sendMessage(user.socket, info, sizeof(info));
        }
    }
    //removes an existing room
    else if (command == "remove" && !command.empty()) {
        std::string removed_room_name = std::move(content);
        if (!validateName(removed_room_name)) {
            const char error[] = "Invalid room name.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else if (!roomExists(removed_room_name)) {
            const char error[] = "This room does not exist.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else if (getRoomPopulation(removed_room_name) != 0) {
            const char error[] = "Cannot remove populated rooms. Wait until the room is empty.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else if (!removeRoom(removed_room_name)) {
            const char error[] = "That room cannot be removed.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else {
            const char info[] = "The room has been removed.\n";
            Socket::sendMessage(user.socket, info, sizeof(info));
        }
    }
    //makes the user join a different room
    else if (command == "join" && !content.empty()) {
        std::string new_room_name = std::move(content);

        if (getLowercase(new_room_name) == getLowercase(room.name)) {
            const char error[] = "You are already in this room.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else if (!roomExists(new_room_name)) {
            const char error[] = "The room specified does not exist.\nYou can create rooms using the /create command\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
            return;
        }
        else {
            auto target_room = findRoom(new_room_name);
            std::string info = "You were moved to the room: " + target_room->second.name + "\n";
            Socket::sendMessage(user.socket, info.c_str(), info.size() + 1);
            moveUser(user, room, target_room->second);
        }
    }
    //renames the user
    else if (command == "rename" && !content.empty()) {
        std::string new_name = std::move(content);
        if (!validateName(new_name)) {
            const char error[] = "Please enter a valid name (only use ASCII digits/letters).\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        if (usernameExists(new_name)) {
            const char error[] = "This name is taken, try a different name.\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else {
            renameUser(user, new_name);
        }
    }
    //sends a message to a single specified user in the room
    else if (command == "w" && !content.empty()) {
        size_t name_end_pos = content.find_first_of(" \t\n\r");
        if (name_end_pos == std::string::npos) {
            const char error[] = "Wrong /w usage. /w format: /w <user> <message>\n";
            Socket::sendMessage(user.socket, error, sizeof(error));
        }
        else {
            std::string target_name = content.substr(0, name_end_pos);
            auto target_user_it = findUser(target_name, room);
            if (target_user_it == room.users.end()) {
                const char error[] = "User not found. Use /users to list all users in this room\n";
                Socket::sendMessage(user.socket, error, sizeof(error));
            }
            else {
                size_t message_start = content.find_first_not_of(" \t\n\r", name_end_pos);
                std::string w_message = content.substr(message_start);
                w_message = "(w)" + user.username + ": " + w_message + "\n";
                Socket::sendMessage(target_user_it->socket, w_message.c_str(), w_message.size() + 1);
            }
        }
    }
    //displays commands
    else if (command == "help") {
        const char help[] = 
        "-----------------------------------------------\n"
        "ChatServer commands:\n"
        "/rooms -> displays all rooms\n"
        "/users -> displays all users in this room\n"
        "/leave -> closes your connection to this server\n"
        "/create <name> -> creates a room with given name\n"
        "/remove <name> -> removes the room with given name\n"
        "/join <name> -> moves you to the room with given name\n"
        "/rename <name> -> changes your name\n"
        "/w <name> <message> -> sends a private message to a user with given name (in this room)\n"
        "none of the above -> sends your message to all users in current room\n"
        "the default room is silent, move to a different one to chat with other users\n"
        "-----------------------------------------------\n";
        Socket::sendMessage(user.socket, help, sizeof(help));
    }
    else {
        const char error[] = "This command is not valid. Use /help to list all commands.\n";
        Socket::sendMessage(user.socket, error, sizeof(error));
    }
}

//sends the contents of @buffer to all users in @room
//does not send to user @sender
void RoomHandler::sendMessageToRoom(const char* buffer, size_t buffer_size, const User& sender, const Room& room) {
    for (auto && user : room.users) {
        if (user.socket != sender.socket) {
            Socket::sendMessage(user.socket, buffer, buffer_size);
        }
    }
}


//////////////////
///ChatServer/////
//////////////////


//waits for actions and processes them
void ChatServer::mainLoop() {

    while (true) {
        populateFDS(); //prepares @fds for select() call
        int activities = Socket::checkActivity(max_socket, &fds);
        if (activities <= 0) {
            continue;
        }

        //checks whether a new user wants to connect
        if (Socket::actionOccured(server_socket, &fds)) {
            acceptConnection();
        }

        //processes actions of existing users
        room_handler.processActions(&fds);
    }
}

//clears @fds, then inserts file descriptors to @fds for each active socket
//sets @max_socket to the highest socket value for select() call
void ChatServer::populateFDS() {
    max_socket = -1;
    Socket::clearFDS(&fds);

    for (auto && room : room_handler.rooms) {
        for (auto && user : room.second.users) {
            Socket::setFDS(user.socket, &fds);
            if (user.socket > max_socket) {
                max_socket = user.socket;
            }
        }
    }
    Socket::setFDS(server_socket, &fds);
    if (server_socket > max_socket) {
        max_socket = server_socket;
    }
}

//accepts new connections
//is nullipotent if connection failed
void ChatServer::acceptConnection() {
    int new_socket;
    if ((new_socket = Socket::getNewClientSocket(server_socket)) == -1) {
        return;
    }

    room_handler.createUser(new_socket);

    const char instructions[] = "Welcome to ChatServer\nEnter your name:\n";
    Socket::sendMessage(new_socket, instructions, sizeof(instructions));
}

int ChatServer::createServerSocket(unsigned int port) {
    server_socket = Socket::createServerSocket(port);
    if (server_socket == -1) {
        return -1;
    }
    return server_socket;
}
