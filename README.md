# Telnet Chatserver

A command line telnet chatserver.

## Features
  - Rooms: Users can create, remove and join different rooms.
  - Whispering: Users can whisper to a specified user. The whispered message will only be shown to the target user, no matter in what room he is.

## Installation

The program is compiled using the ```make``` command.

The repository can be cleaned using ```make clean```.

## Usage

```./bin/ChatServer [port]```

The above command will start the chatserver on the specified port.
Users can connect via the following command:

```telnet localhost [port]```

Users can view the list of commands by typing ```/help```.
