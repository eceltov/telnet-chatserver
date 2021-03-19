#include "socket.h"
#include "chatserver.h"

int main() {
    ChatServer chatserver;
    chatserver.createServerSocket(9999);
    /*if (resp == -1) {
        close(9999);
        chatserver.createServerSocket(9999);
    }*/
    
    chatserver.mainLoop();



    return 0;
}