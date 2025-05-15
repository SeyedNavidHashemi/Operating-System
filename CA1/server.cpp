#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include "room.h"
using namespace std;

typedef struct pollfd pollfd;

#define STDIN 0
#define STDOUT 1
#define BUFFER_SIZE 1024

const char* SERVER_LAUNCHED = "Server Launched!\n";
const char* NEW_CONNECTION = "New Connection!\n";
const std::string PLAY_AGAIN = "Play again";
const std::string SEND = "&*";
const std::string END_GAME = "end_game";
const char* BROADCAST_IP = "127.255.255.255";
const int SERVER_PORT = 6060; 
const int ROOM_BASE_PORT = 2007;

struct Player {
    std::string name;
    int current_room = -1;
    int file_descriptor;
    int points;
    int answer = 0;
    int status;
    Player(int fd)
        : file_descriptor(fd), points(0), status(0) {}
};
std::vector<Player> players;

int handle_the_recieve_message(std::string buffer)
{
    if(buffer.substr(0, SEND.length()) == SEND)
        return 1;
    if(buffer.substr(0, PLAY_AGAIN.length()) == PLAY_AGAIN)
        return 2;
    if(buffer.substr(0, END_GAME.length()) == END_GAME)
        return 3;
    return -1;
}

std::string show_list_of_empty_rooms(std::vector<Room>& rooms, int num_of_rooms)
{
    std::vector<int> empty_rooms;
    std::ostringstream oss;
    oss << "Empty rooms are:\n";

    for(int i = 0; i < num_of_rooms; i++)
    {
        if(rooms[i].which_one_is_empty() > 0)
        {
            int room_num = rooms[i].get_room_number();
            empty_rooms.push_back(room_num);
            oss << "Room: " << room_num << "\n";
        }
    }

    oss << "Please choose one of them to join the game:\n";
    return oss.str();
}

void check_for_name(std::vector<Room>& rooms, int num_of_rooms, const char* command, int fd)
{
    char welcome[BUFFER_SIZE];
    if(command == nullptr)
    {
        sprintf(welcome, "Please start your name by: ");
        send(fd, welcome, strlen(welcome), 0);
        return;
    }

    const char* name = command;
    if(strlen(name) == 0)
        return;

    for(int i = 0; i < players.size(); i++)
    {
        if(players[i].name == name)
        {
            write(1, "This name is duplicated, please enter another name.\n", 0);
            return;
        }
        if(players[i].file_descriptor == fd)
        {
            sprintf(welcome, "Welcome to this game %s", name);
            send(fd, welcome, strlen(welcome), 0);
            std::string empty_rooms = show_list_of_empty_rooms(rooms, num_of_rooms);
            send(fd, empty_rooms.c_str(), empty_rooms.size(), 0);
            players[i].name = name;
            players[i].status = 1;
        }
    }
}

void create_rooms(std::vector<Room>& rooms, int num_of_rooms)
{
    int room_port = ROOM_BASE_PORT;
    for (int i = 0; i < num_of_rooms ; i++)
    {
        Room new_room(room_port, i);
        rooms.push_back(new_room);
        room_port++;
    }
}

int get_status_of_player(int fd)
{
    for(auto& player : players)
    {
        if(player.file_descriptor == fd)
        {
            return player.status;
        }
    }
    return -1;
}

void set_status_of_player(int fd, int new_status)
{
    for(int i = 0; i < players.size(); i++)
    {
        if(players[i].file_descriptor == fd)
        {
            players[i].status = new_status;
        }
    }
}

bool check_selected_room(const char* buffer, std::vector<Room>& rooms)
{
    int room_num = atoi(buffer);
    for(int i = 0; i < rooms.size(); i++)
    {
        if(i == room_num)
        {
            if(rooms[i].which_one_is_empty() > 0)
            {
                return true;
            }
        }
    }
    return false;
}

int room_port_from_number(std::vector<Room>& rooms, const char* buffer)
{
    int room_num = atoi(buffer);
    for(int i = 0; i < rooms.size(); i++)
    {
        if(rooms[i].get_room_number() == room_num)
        {
            return rooms[i].get_room_port();
        }
    }
    return -1;
}

void ask_room_to_accept_this_player(std::vector<Room>& rooms, const char* buffer, int player_fd)
{
    int room_num = atoi(buffer);
    for(int i = 0; i < rooms.size(); i++)
    {
        if(rooms[i].get_room_number() == room_num)
        {
            rooms[i].accept_this_client(player_fd);
        }
    }
}

void set_player_currecnt_room(int player_room, int fd)
{
    for(int i = 0; i < players.size(); i++)
    {
        if(players[i].file_descriptor == fd)
        {
            players[i].current_room = player_room;
        }
    }
}

int get_player_current_room(int player_fd)
{
    for(int i = 0; i < players.size(); i++)
    {
        if(players[i].file_descriptor == player_fd)
            return players[i].current_room;
    }
    return -1;
}

std::string update_winnings(int res, int room_num)
{
    if(res == 0)
    {
        std::string draw = "draw";
        char result[BUFFER_SIZE];
        memset(result, 0, 100);
        sprintf(result, "Server: Game in room %d was DRAW.\nRoom %d is also available.\n", room_num, room_num);
        return result;
    }
    else
    {
        std::string result_win;
        for(int i = 0; i < players.size(); i++)
        {
            if(players[i].current_room == room_num)
                players[i].current_room = -1;
        }
        for(int i = 0; i < players.size(); i++)
        {
            if(players[i].file_descriptor == res)
            {
                players[i].points++;
                result_win = "Server: Game in room " + to_string(room_num) + " WINNER is " + players[i].name 
                            + "Room " + to_string(room_num) + " is also available.\n";  
            }
        }
        return result_win;
    }
}

int create_broad_cast_socket(struct sockaddr_in& broadcast_addr)
{
    int broadcast_fd;
    int opt = 1;
    if((broadcast_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
        perror("FAILED: Socket was not created");
    int broadcast_permission = 1;
    if(setsockopt(broadcast_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission)) < 0)
        perror("FAILED: permission not granted");
    if(setsockopt(broadcast_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        perror("FAILED: permission not granted");
    memset(broadcast_addr.sin_zero, 0, sizeof(broadcast_addr.sin_zero));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(SERVER_PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
    bind(broadcast_fd, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
    return broadcast_fd;
}

std::string prepare_final_list()
{
    std::string result = "Player Information:\n\n";
    for(int i = 0; i < players.size(); i++)
    {
        result += players[i].name + "wins: " + to_string(players[i].points)
                 + " \npoints: " + to_string(players[i].points) + "\n\n";
    }
    return result;
}

int main(int argc, char* argv[])
{
    if(argc != 4)
        perror("Invalid Arguments");

    int num_of_rooms = std::stoi(argv[3]);
    char* ipaddr = argv[1];
    struct sockaddr_in server_addr, broad_addr;
    int server_fd, broad_fd, opt = 1;
    std::vector<pollfd> pfds;
    
    //here is about creating rooms
    std::vector<Room> rooms;
    std::vector<int> rooms_recv(num_of_rooms, 0);


    broad_fd =create_broad_cast_socket(broad_addr);

    server_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, ipaddr, &(server_addr.sin_addr)) == -1)
        perror("FAILED: Input ipv4 address invalid");

    if((server_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        perror("FAILED: Socket was not created");

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable failed");

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable port failed");

    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    
    server_addr.sin_port = htons(strtol(argv[2], NULL, 10));

    if(bind(server_fd, (const struct sockaddr*)(&server_addr), sizeof(server_addr)) == -1)
        perror("FAILED: Bind unsuccessfull");

    if(listen(server_fd, 20) == -1)
        perror("FAILED: Listen unsuccessfull");

    write(1, SERVER_LAUNCHED, strlen(SERVER_LAUNCHED));

    pfds.push_back(pollfd{STDIN, POLLIN, 0});
    pfds.push_back(pollfd{server_fd, POLLIN, 0});

    create_rooms(rooms, num_of_rooms);
    while(1)
    {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        if(poll(pfds.data(), (nfds_t)(pfds.size()), -1) == -1)
            perror("FAILED: Poll");   
        
        for(size_t i = 0; i < pfds.size(); ++i)
        {
            if(pfds[i].revents & POLLIN)
            {
                if(pfds[i].fd == server_fd) // new user
                {
                    struct sockaddr_in new_addr;
                    socklen_t new_size = sizeof(new_addr);
                    int new_fd = accept(server_fd, (struct sockaddr*)(&new_addr), &new_size);
                    write(1, NEW_CONNECTION, strlen(NEW_CONNECTION));
                    Player new_player(new_fd);
                    players.push_back(new_player);
                    char wel_mes[] = "Welcome to this game!\nPlease provide your name:\n";
                    send(new_fd, wel_mes, strlen(wel_mes), 0);
                    pfds.push_back(pollfd{new_fd, POLLIN, 0});
                }
                if(pfds[i].fd == STDIN)
                {
                    int read_bytes = read(STDIN, buffer, BUFFER_SIZE);
                    if(read_bytes > 0)
                    {
                        if((handle_the_recieve_message(buffer) == 3))
                        {
                            std::string result = prepare_final_list();
                            sendto(broad_fd, result.c_str(), strlen(result.c_str()), 0, (struct sockaddr*)&broad_addr, sizeof(broad_addr));
                            exit(0);
                        }
                    }
                }
                else 
                {   
                    switch(get_status_of_player(pfds[i].fd))
                    {
                        case 0:
                        {
                            recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                            check_for_name(rooms, num_of_rooms, buffer, pfds[i].fd);
                            break;
                        }
                        //check emptiness of selected room
                        case 1:
                        {
                            recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                            if(!check_selected_room(buffer, rooms))
                            {
                                send(pfds[i].fd, "It has no space!\n", strlen("It has no space!"), 0);
                            }
                            else
                            {
                                int room_port = room_port_from_number(rooms, buffer);
                                int room_num = atoi(buffer);
                                set_player_currecnt_room(room_num, pfds[i].fd);
                                char port[12];
                                sprintf(port, "port:%d", room_port);
                                send(pfds[i].fd, port, strlen(port), 0);
                                ask_room_to_accept_this_player(rooms, buffer, pfds[i].fd);
                                set_status_of_player(pfds[i].fd, 2);
                            }
                            break;
                        }
                        case 2:
                        {
                            recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                            int result;
                            if(handle_the_recieve_message(buffer) == 1)
                            {
                                int room_num = get_player_current_room(pfds[i].fd);
                                for(int i = 0; i < rooms.size(); i++)
                                {
                                    if(rooms[i].get_room_number() == room_num)
                                        rooms_recv[i]++;
                                    if(rooms_recv[i] == 2)
                                    {
                                        result = rooms[i].recieve_from_players();
                                        rooms_recv[i] = 0;
                                        rooms[i].initialize_the_room();
                                        std::string res = update_winnings(result, room_num);
                                        sendto(broad_fd, res.c_str(), strlen(res.c_str()), 0, (struct sockaddr*)&broad_addr, sizeof(broad_addr));
                                    }
                                }
                            }
                            set_status_of_player(pfds[i].fd, 3);
                            break;
                        }
                        case 3:
                        {
                            recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                            if(handle_the_recieve_message(buffer) == 2)
                            {
                                std::string empty_rooms = show_list_of_empty_rooms(rooms, num_of_rooms);
                                send(pfds[i].fd, empty_rooms.c_str(), empty_rooms.size(), 0);
                                set_status_of_player(pfds[i].fd, 1);
                            }
                            break;
                        }
                        // message from user
                        default:
                        {
                            int recv_bytes = recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                            if(recv_bytes > 0)
                            {
                                (pfds[i].fd);
                            }
                            break;
                        }
                    }
                }
            }
            //for checking starting game in rooms
            for(int i = 0; i < rooms.size(); i++)
            {
                if(rooms[i].is_ready_to_start())
                    rooms[i].start_the_game();
            }
        }

    }
}