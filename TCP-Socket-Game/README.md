# Rock Paper Scissors - Multiplayer Game Server

A fully-featured multiplayer Rock Paper Scissors game server implemented in C++ with multi-room support and real-time game sessions.

## 🎮 Overview

This project implements a robust client-server architecture for playing Rock Paper Scissors in multiplayer mode. The system supports multiple concurrent game rooms where players can compete against each other. The server manages player connections, room assignments, game logic, and real-time communication using TCP sockets and broadcast messaging.

## ✨ Features

- **Multi-Room Support**: Create and manage multiple game rooms simultaneously
- **Real-Time Communication**: TCP-based communication between server and clients
- **Broadcast Notifications**: UDP broadcast for game results and announcements
- **Concurrent Players**: Support for multiple players across different rooms
- **Room Management**: Automatic room assignment and player matching
- **Timeout Handling**: 10-second timeout for player moves
- **Score Tracking**: Track wins and maintain player statistics
- **Persistent Sessions**: Players can play multiple rounds

## 🏗️ Architecture

### Components

- **Server** (`server.cpp`): Main server application managing rooms, players, and game sessions
- **Room** (`room.cpp`, `room.h`): Room management and game logic for each game room
- **Client** (`client.cpp`): Client application for players to connect and play

### System Design

- **Protocol**: TCP/IP for reliable client-server communication
- **Broadcast**: UDP broadcast for game-wide announcements
- **Concurrency**: Poll-based I/O for handling multiple connections
- **State Management**: Status-based player flow (login → room selection → game → results)

## 🚀 Getting Started

### Prerequisites

- C++11 or higher
- Make utility
- POSIX-compliant system (Linux/Unix/macOS)

### Build Instructions

1. Clone the repository:
```bash
git clone <repository-url>
cd Operating-System-main/CA1
```

2. Compile the project:
```bash
make
```

This will generate the `server` executable.

3. Run the server:
```bash
./server <IP_ADDRESS> <SERVER_PORT> <NUMBER_OF_ROOMS>
```

Example:
```bash
./server 127.0.0.1 8080 3
```

4. Run clients (in separate terminals):
```bash
./client <IP_ADDRESS> <PORT>
```

## 📋 Usage

### Server

The server accepts three parameters:
1. **IP Address**: The IP address to bind the server
2. **Server Port**: The port number for client connections
3. **Number of Rooms**: Total number of game rooms to create

### Client

Players connect to the server using:
1. Enter a unique player name
2. Select an available room
3. Wait for another player to join
4. Make a move (Rock: 1, Paper: 2, Scissors: 3) within 10 seconds
5. View results and optionally play again

### Game Flow

1. **Connection**: Client connects to server
2. **Authentication**: Player provides a unique name
3. **Room Selection**: Choose from available empty rooms
4. **Matchmaking**: Wait for second player to join
5. **Gameplay**: Make simultaneous moves with timeout
6. **Results**: View winner, scores, and continue playing

## 🎯 Technical Details

### Key Technologies

- **C++11**: Modern C++ features and standard library
- **Socket Programming**: TCP for connections, UDP for broadcasts
- **I/O Multiplexing**: `poll()` system call for efficient I/O handling
- **Network Protocols**: IPv4 addressing and port management

### Data Structures

- Vector-based player and room management
- Poll-based file descriptor tracking
- Status-based player state machine

### Communication Protocol

- **TCP**: Reliable message delivery between server and clients
- **UDP Broadcast**: Game results and system-wide announcements
- **Message Format**: Text-based commands and responses

## 🛠️ Code Structure

```
├── server.cpp      # Main server implementation
├── room.cpp        # Room class implementation
├── room.h          # Room class header
├── client.cpp      # Client application
├── Makefile        # Build configuration
└── README.md       # Project documentation
```

## 🎓 Learning Outcomes

This project demonstrates proficiency in:

- **System Programming**: Operating system concepts and system calls
- **Network Programming**: Socket programming and network protocols
- **Concurrent Programming**: Managing multiple simultaneous connections
- **Game Design**: Real-time multiplayer game architecture
- **C++ Development**: Object-oriented programming and modern C++ practices

## 📝 Future Enhancements

- [ ] Spectator mode for watching games
- [ ] Tournament brackets
- [ ] Player rankings and leaderboards
- [ ] Chat functionality between players
- [ ] Database integration for persistent stats
- [ ] Web-based client interface

## 📄 License

This project is part of an academic coursework (CA1) focused on Operating Systems concepts.

## 🙏 Acknowledgments

Developed as part of an Operating Systems course assignment focusing on network programming and system architecture.

