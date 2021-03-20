#include "socket.h"



fd_set Socket::fds; //is it neccessary? yes it is

int Socket::createServerSocket(int port) {
    //try to create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::cerr << "Server socket could not be established" << std::endl;
        return -1;
    }

    //structure for binding the socket to port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    //bind the socket to local address
    int bind_status = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    if (bind_status == -1)
    {
        std::cerr << "Error binding socket to local address" << std::endl;
        return -1;
    }

    int listen_status = listen(server_socket, SOMAXCONN);
    if (listen_status == -1) {
        close(server_socket);
        return -1;
    }

    std::cout << "Waiting for connections..." << std::endl;

    return server_socket;
}

//blocking
int Socket::getNewClientSocket(int server_socket) {
    struct sockaddr_in client_address;
    socklen_t address_length = sizeof(client_address);

    int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &address_length);
    if (client_socket == -1) {
        return -1;
    }

    return client_socket;
}

int Socket::closeSocket(int socket) {
    return close(socket);
}

//blocking
int Socket::checkActivity(std::set<int> & sockets) {
    FD_ZERO(&fds); //clear the set

    if (sockets.empty()) {
        return -1;
    }

    std::set<int>::iterator it; //populate the set with current sockets
    for (it = sockets.begin(); it != sockets.end(); it++) {
        FD_SET(*it, &fds);
    }
    
    int last_socket = *sockets.rbegin();

    int activities = select(last_socket + 1, &fds, NULL, NULL, NULL); //check for activity (blocking)
    return activities;
}

//checks if socket remained after select() call
int Socket::actionOccured(int socket) {
    return FD_ISSET(socket, &fds);
}

int Socket::getMessage(int socket, char* buffer, size_t buffer_size) {
    return read(socket, buffer, buffer_size);
}

int Socket::sendMessage(int socket, const char* buffer, size_t buffer_size) {
    return write(socket, buffer, buffer_size);
}