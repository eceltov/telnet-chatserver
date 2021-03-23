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


/**
 * Represents a user of the chat server.
 */
struct User {

    User(int new_socket) : socket(new_socket) {}

    std::string username;
    bool has_name = false;
    int socket;
    bool leaving = false; //true if user want to leave the server
};

/**
 * Represents a room of the chat server.
 * Contains users, stores which users are to be removed in the next iteration
 * Can be silent (users cannot chat openly)
 * Can be removable (the room can be removed by users)
 */
struct Room {

    Room(const std::string& name_): name(name_) {};

    std::list<User> users;
    std::set<int> removal_sockets; //sockets of users that are to be removed
    size_t user_count = 0;
    std::string name;
    bool silent = false; //if true, users cannot openly chat
    bool removable = true; //if true, the room cannot be removed via user commands
};

/**
 * Stores rooms and provides methods for room and user manipulation.
 * Stores metadata (names, sockets) about rooms and users. 
 * Creates a default room at initialization
 */
struct RoomHandler {

    RoomHandler(std::string& default_room_name_);

    bool roomExists(const std::string& room_name);
    bool roomNameTaken(const std::string& room_name);
    void addNewRoomEntry(const std::string& room_name);
    bool removeRoom(const std::string& room_name);
    size_t getRoomPopulation(const std::string& room_name);
    void processNewRoomEntries();
    void moveUser(User& user, Room& source, Room& target);
    void createUser(int user_socket);
    void removeUser(User& user, Room& source);
    void nameUser(User& user, const std::string& username);
    void renameUser(User& user, const std::string& new_username);
    bool usernameExists(const std::string& username);
    void processUserRemoval(Room& source);
    void processActions(fd_set* fds);
    void sendMessageToRoom(const char* buffer, size_t buffer_size, const User& sender, const Room& room);
    void processMessage(std::string& message, User& user, Room& room);
    std::map<std::string, Room>::iterator findRoom(const std::string& room_name);
    std::list<User>::const_iterator findUser(const std::string& username, const Room& source) const;

    std::string default_room_name;
    std::map<std::string, Room> rooms; //lowercase name -> Room
    std::stack<Room> new_rooms; //rooms to be added in next iteration
    std::set<std::string> room_names; //lowercase room names (including rooms to be added)
    std::set<std::string> usernames;
};

/**
 * Handles new connections and checks for user activity
 * If an user activity occured, the users file descriptor will be stored in @fds
 */
class ChatServer {
  public:

    ChatServer(std::string& default_room_name_)
    : room_handler(default_room_name_), default_room_name(default_room_name_) {}

    ~ChatServer() {
        for (auto && room : room_handler.rooms) {
          for (auto && user : room.second.users) {
            Socket::closeSocket(user.socket);
          }
        }
    }

    void mainLoop();
    int createServerSocket(int port);
    void populateFDS();

    RoomHandler room_handler;
    fd_set fds;
    int max_socket; //required for select() call

  private:

    void acceptConnection();

    int server_socket;
    std::string default_room_name;
};

#endif
