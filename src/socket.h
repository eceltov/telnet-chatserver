#ifndef __socket__
#define __socket__

#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

#include <set>
#include <iostream>

namespace Socket {
    int createServerSocket(int port);
    int getNewClientSocket(int serverSocket);
    int closeSocket(int socket);
    int checkActivity(int max_socket, fd_set* fds);
    int actionOccured(int socket, fd_set* fds);
    int getMessage(int socket, char* buffer, size_t buffer_size);
    int sendMessage(int socket, const char* buffer, size_t buffer_size);
    void setFDS(int socket, fd_set* fds);
    void clearFDS(fd_set* fds);
};

#endif
