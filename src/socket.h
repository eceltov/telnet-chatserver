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


class Socket {
  public:

    static int createServerSocket(int port);
    static int getNewClientSocket(int serverSocket);
    static int checkActivity(std::set<int> & sockets);
    static int actionOccured(int socket);
    static int getMessage(int socket, char* buffer, size_t buffer_size);
    static int sendMessage(int socket, const char* buffer, size_t buffer_size);

  private:

    static fd_set fds;
};


#endif