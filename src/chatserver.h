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
    bool leaving = false; //true if user want to leave the server

    bool operator==(const User& other) {
      return this->socket == other.socket;
    }

};

struct RoomHandler;

struct Room {
  public:
    Room(std::string name_, RoomHandler* room_handler_): name(name_), room_handler(room_handler_) {};

    void sendMessageToOthers(const char* buffer, size_t buffer_size, const User & sender);
    void checkAndProcessMessages();
    void processMessage(std::string & message, User & user);

    std::list<User> users;
    std::set<int> removal_sockets;
    size_t user_count = 0;
    std::string name;
    bool silent = false;
    bool removable = true;
    RoomHandler* room_handler;

  private: ////
};

struct RoomHandler {

  RoomHandler(std::string& default_room_name, std::set<int>* sockets_);

  std::set<int>* sockets;
  std::map<std::string, Room> rooms; //lowercase name -> Room
  std::stack<Room> new_rooms; //rooms to be added in next iteration
  std::set<std::string> room_names; //lowercase room names (including rooms to be added)
  std::set<std::string> usernames;

  bool roomExists(const std::string& room_name); //checks if Room already exists
  bool roomNameTaken(const std::string& room_name); //checks if a room already has this name (including new ones)
  void addNewRoomEntry(const std::string& room_name);
  bool removeRoom(const std::string& room_name);
  size_t getRoomPopulation(const std::string& room_name);
  void processNewRoomEntries();
  void moveUser(User& user, Room& source, Room& target);
  void createUser(int user_socket, Room& target);
  void removeUser(User& user, Room& source);
  void nameUser(User& user, const std::string& username);
  void renameUser(User& user, const std::string& new_username);
  bool usernameExists(const std::string& username);
  void processUserRemoval(Room& source);
  void processActions();

  std::map<std::string, Room>::iterator findRoom(const std::string& room_name);
  std::list<User>::const_iterator findUser(const std::string& username, const Room& source) const;
};

class ChatServer {
  public:

    ChatServer(std::string& default_room_name_)
    : room_handler(default_room_name_, &sockets), default_room_name(default_room_name_) {}

    ~ChatServer() {
        for (auto&& socket : sockets) {
          Socket::closeSocket(socket);
        }
    }

    void mainLoop();
    void acceptConnection();
    int createServerSocket(int port);

    RoomHandler room_handler;
  private:
    int server_socket;
    std::set<int> sockets;
    std::string default_room_name;
};



#endif