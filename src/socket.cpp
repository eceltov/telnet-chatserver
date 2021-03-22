#include "socket.h"

int Socket::createServerSocket(int port) {
    //try to create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::cerr << "Server socket could not be established." << std::endl;
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
        std::cerr << "Error binding socket to local address." << std::endl;
        std::cerr << "If you closed the server recently, try waiting a few minutes." << std::endl;
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

//makes @server_socket wait until a new client attempts to connect
//blocking, use after an action occured
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

void Socket::setFDS(int socket, fd_set* fds) {
    FD_SET(socket, fds);
} 

void Socket::clearFDS(fd_set* fds) {
    FD_ZERO(fds);
}

//checks if socket remained after select() call
int Socket::actionOccured(int socket, fd_set* fds) {
    return FD_ISSET(socket, fds);
}

//checks for socket activities
//blocking
int Socket::checkActivity(int max_socket, fd_set* fds) {
    int activities = select(max_socket + 1, fds, NULL, NULL, NULL); //check for activity (blocking)
    return activities;
}

int Socket::getMessage(int socket, char* buffer, size_t buffer_size) {
    return read(socket, buffer, buffer_size);
}

int Socket::sendMessage(int socket, const char* buffer, size_t buffer_size) {
    return write(socket, buffer, buffer_size);
}
