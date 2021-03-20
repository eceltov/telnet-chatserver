#include "socket.h"
#include "chatserver.h"

int main() {
    std::string default_room_name = "Silent";
    ChatServer chatserver(default_room_name);
    chatserver.createServerSocket(9999);
    
    chatserver.mainLoop();



    return 0;
}