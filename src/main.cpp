#include "socket.h"
#include "chatserver.h"

#include <iostream>

int main() {
    unsigned int port = 9999;
    //std::cout << "Please enter a port number for the server:" << std::endl;
    //std::cin >> port;
    std::string default_room_name = "Silent";
    ChatServer chatserver(default_room_name);
    if (chatserver.createServerSocket(port) < 0) {
        std::cerr << "Failed to create server socket." << std::endl;
        return 1;
    }
    chatserver.mainLoop();
    return 0;
}
