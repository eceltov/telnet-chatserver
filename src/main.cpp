#include "socket.h"
#include "chatserver.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    int port;
    if (argc == 1) {
        std::cout << "Please enter a port number for the server:" << std::endl;
        std::cin >> port;
    }
    else {
        try {
            port = std::stoi(argv[1]);
        }
        catch (...) {
            std::cerr << "error: invalid port number." << std::endl;
            return 1;
        }
    }
    
    std::string default_room_name = "Silent";
    ChatServer chatserver(default_room_name);
    if (chatserver.createServerSocket(port) < 0) {
        std::cerr << "Failed to create server socket." << std::endl;
        return 1;
    }
    chatserver.mainLoop();
    return 0;
}
