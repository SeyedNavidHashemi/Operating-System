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

typedef struct pollfd pollfd;

#define STDOUT 1
#define BUFFER_SIZE 1024
const char* SERVER_IP = "127.0.0.1";
const std::string ROCK = "1";
const std::string PAPER = "2";
const std::string SCISSORS = "3";
const std::string NO_ANSWER = "*";

int make_the_result(std::string first_move, std::string second_move)
{
    int first_act; 
    int second_act;

    if(first_move.substr(0, ROCK.length()) == ROCK)
        first_act = 1;
    else if(first_move.substr(0, PAPER.length()) == PAPER)
        first_act = 2;
    else if(first_move.substr(0, SCISSORS.length()) == SCISSORS)
        first_act = 3;
    else if(first_move.substr(0, NO_ANSWER.length()) == NO_ANSWER)
        first_act = 0;

    if(second_move.substr(0, ROCK.length()) == ROCK)
        second_act = 1;
    else if(second_move.substr(0, PAPER.length()) == PAPER)
        second_act = 2;
    else if(second_move.substr(0, SCISSORS.length()) == SCISSORS)
        second_act = 3;
    else if(second_move.substr(0, NO_ANSWER.length()) == NO_ANSWER)
        second_act = 0;

    if((first_act == 0) && (second_act == 0))
        return 0;
    if(first_act == 0)
        return 2;
    if(second_act == 0)
        return 1;
    if(((first_act == 1) && (second_act == 3)) || ((first_act == 2) && (second_act == 1)) || ((first_act == 3) && (second_act == 2)))
        return 1;
    else
        return 2;
}

Room::Room(int room_p, int room_n)
{
    first_fd = -1;
    ser_first = -1;
    second_fd = -1;
    ser_second = -1;
    started = 1;
    room_port = room_p;
    room_num = room_n;

    struct sockaddr_in room_addr;
    int room_fd, opt = 1;

    room_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, SERVER_IP, &(room_addr.sin_addr)) == -1)
        perror("FAILED: Input ipv4 address invalid");

    if((room_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        perror("FAILED: Socket was not created");
    if(setsockopt(room_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable failed");

    if(setsockopt(room_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable port failed");

    memset(room_addr.sin_zero, 0, sizeof(room_addr.sin_zero));
    
    room_addr.sin_port = htons(room_p);

    if(bind(room_fd, (const struct sockaddr*)(&room_addr), sizeof(room_addr)) == -1)
        perror("FAILED: Bind unsuccessfull");

    if(listen(room_fd, 2) == -1)
        perror("FAILED: Listen unsuccessfull");
    room_file_d = room_fd;
    room_str = room_addr;
}

int Room::which_one_is_empty(){
    if(first_fd == -1)
        return 1;
    if(second_fd == -1)
        return 2;
    else
        return 0;
}

void Room::accept_this_client(int file_descirptor)
{
    socklen_t new_size = sizeof(room_str);
    int new_fd = accept(room_file_d, (struct sockaddr*)(&room_str), &new_size);
    if(first_fd == -1)
    {
        first_fd = new_fd;
        ser_first = file_descirptor;
    }
    else if(second_fd == -1)
    {
        second_fd = new_fd;
        ser_second = file_descirptor;
        
    }
    char room_wel[BUFFER_SIZE];
    sprintf(room_wel, "Welcome to room %d! Please wait for the next player...\n", room_num);

    if((first_fd == -1) || (second_fd == -1))
        send(new_fd, room_wel, strlen(room_wel), 0);
}

bool Room::is_ready_to_start()
{
    if((first_fd != -1) && (second_fd != -1) && (started == 1))
        return true;
    else return false;
}

void Room::start_the_game()
{
    started = 0;
    char select_mess[BUFFER_SIZE];
    sprintf(select_mess, "Choose one of these in 10 sec\nRock : 1\nPaper : 2\nScissors : 3\n", BUFFER_SIZE);
    send(first_fd, select_mess, strlen(select_mess), 0);
    send(second_fd, select_mess, strlen(select_mess), 0);

}

void Room::initialize_the_room()
{
    first_fd = -1;
    ser_first = -1;
    second_fd = -1;
    ser_second = -1;
    started = 1;
}

int Room::recieve_from_players()
{
    char first_move[5];
    memset(first_move, 0, 5);
    char second_move[5];
    memset(second_move, 0, 5);

    recv(first_fd, first_move, 2, 0);
    recv(second_fd, second_move, 2, 0);

    int temp_result = make_the_result(first_move, second_move);
    int result;
    switch (temp_result)
    {
    case 0:
        result = 0;
        break;
    case 1:
        result = ser_first;
        break;
    case 2:
        result = ser_second;
        break;
    default:
        result = 0;
        break;
    }
    return result;
}
