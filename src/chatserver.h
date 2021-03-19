#ifndef __chatserver__
#define __chatserver__

#include <iostream>
#include <string>
#include <set>
#include <map>
#include <sstream>
#include <utility>
#include <vector>
#include <list>
#include <stack>

struct User {
  public:

    User(int new_socket) : socket(new_socket) {}

    std::string username;
    bool has_name = false;
    int socket;
    //bool creating_room = false;
    //std::string created_room_name = "";

    bool operator==(const User& other) {
      return this->socket == other.socket;
    }

};

struct Room {
  public:
    Room(std::string name_, std::map<std::string, Room>* rooms_, std::stack<Room>* new_rooms_)
    : name(name_), rooms(rooms_), new_rooms(new_rooms_) {};

    void addUser(User && user);
    void sendMessageToOthers(const char* buffer, size_t buffer_size, const User & sender);
    void checkAndProcessMessages();
    void processMessage(std::string && message, User & user);
    bool validateName(const std::string & name); 
    void sanitizeMessage(std::string& message);
    void removeUser(int socket);
    void moveUser(User & user, std::string & new_room_name);
    void processRemoval();

    //std::set<int> user_sockets;
    std::list<User> users;
    std::set<int> removal_sockets;
    size_t user_count = 0;
    std::string name;
    std::map<std::string, Room>* rooms;
    std::stack<Room>* new_rooms;
    bool silent = false;

  private: ////
};

class ChatServer {
  public:

    ChatServer() {
      Room silent_room("Silent", &rooms, &new_rooms);
      silent_room.silent = true;
      rooms.insert({"Silent", silent_room});
    }

    ~ChatServer() {
        close(server_socket); //mby close other sockets too?
    }

    void mainLoop();
    void acceptConnection();
    int createServerSocket(int port);
  private:
    int server_socket;
    std::set<int> sockets;
    std::map<std::string, Room> rooms;
    std::stack<Room> new_rooms;
};



#endif