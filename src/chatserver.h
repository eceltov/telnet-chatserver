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
#include <algorithm>
#include <cctype>

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

struct RoomHandler;

struct Room {
  public:
    Room(std::string name_, RoomHandler* room_handler_): name(name_), room_handler(room_handler_) {};

    void addUser(User && user);
    void sendMessageToOthers(const char* buffer, size_t buffer_size, const User & sender);
    void checkAndProcessMessages();
    void processMessage(std::string && message, User & user);
    bool validateName(const std::string & name); 
    void sanitizeMessage(std::string& message);
    void removeUser(const User& user);
    //void moveUser(User & user, std::string & new_room_name);
    void processRemoval();

    //std::set<int> user_sockets;
    std::list<User> users;
    std::set<int> removal_sockets;
    size_t user_count = 0;
    std::string name;
    //std::map<std::string, Room>* rooms;
    //std::stack<Room>* new_rooms;
    bool silent = false;
    RoomHandler* room_handler;

  private: ////
};

struct RoomHandler {
  std::map<std::string, Room> rooms; //lowercase name -> Room
  std::stack<Room> new_rooms; //rooms to be added in next iteration
  std::set<std::string> room_names; //lowercase room names (including rooms to be added)
  //TODO: automatically add room_name (should contain lowercase method)

  bool roomExists(const std::string& room_name); //checks if Room already exists
  bool roomNameTaken(const std::string& room_name); //checks if a room already has this name (including new ones)
  void addNewRoomEntry(const std::string& room_name);
  void processNewRoomEntries();
  void moveUser(User& user, Room& source, Room& target);
  std::map<std::string, Room>::iterator findRoom(const std::string& room_name);
};

class ChatServer {
  public:

    ChatServer() {
      Room silent_room("Silent", &room_handler);
      silent_room.silent = true;
      room_handler.rooms.insert({"silent", silent_room});
    }

    ~ChatServer() {
        close(server_socket); //mby close other sockets too?
    }

    void mainLoop();
    void acceptConnection();
    int createServerSocket(int port);

    RoomHandler room_handler;
  private:
    int server_socket;
    std::set<int> sockets;
};



#endif