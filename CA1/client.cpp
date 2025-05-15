#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <poll.h>
#include <iostream>
#include <csignal>
#include <sys/time.h>
#include <sys/select.h> 
typedef struct pollfd pollfd;

#define STDIN 0
#define STDOUT 1
#define BUFFER_SIZE 1024
bool response_recieved = false;
int room_fd = 0;
int server_file_des = 0;
const std::string NO_ANSWER = "*";
const std::string CHOOSE = "Choose";
const std::string PLAYER_INF = "Player Information:";
const int SERVER_PORT = 6060; 
const char* BROADCAST_IP = "127.255.255.255";

int handle_the_recieve_message(std::string buffer)
{
    if(buffer.substr(0, CHOOSE.length()) == CHOOSE)
        return 1;
    if(buffer.substr(0, PLAYER_INF.length()) == PLAYER_INF)
        return 2;
    return -1;
}

int create_broadcast_socket(struct sockaddr_in&server_broadcast_addr)
{
    int file_des;
    int broadcast = 1, opt = 1;
    if((file_des = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        perror("FAILED: Socket was not created");
    setsockopt(file_des, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(file_des, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    memset(&server_broadcast_addr, 0, sizeof(server_broadcast_addr));
    server_broadcast_addr.sin_family = AF_INET;
    server_broadcast_addr.sin_port = htons(SERVER_PORT);
    server_broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
    if(bind(file_des, (struct sockaddr*)&server_broadcast_addr, sizeof(server_broadcast_addr)) < 0)
        perror("FAILED: binding failed");
    return file_des;
}

int main(int argc, char* argv[])
{
    if(argc != 3)
        perror("Invalid Arguments");

    char* ipaddr = argv[1];
    struct sockaddr_in server_addr, broadcast_addr;
    int server_fd, broad_fd, opt = 1;

    int broadcast = 1;
    if((broad_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        perror("FAILED: Socket was not created");
    setsockopt(broad_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(broad_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(SERVER_PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
    if(bind(broad_fd, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) < 0)
        perror("FAILED: binding failed");

    socklen_t broadcast_len = sizeof(broadcast_addr);

    server_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, ipaddr, &(server_addr.sin_addr)) == -1)
        perror("FAILED: Input ipv4 address invalid");

    if((server_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        perror("FAILED: Socket was not created");

    server_file_des = server_fd;

    //here is about polling
    struct pollfd pfds[4];
    pfds[0].fd = STDIN;
    pfds[1].fd = server_fd;
    pfds[2].fd = -1;
    pfds[3].fd = broad_fd;
    pfds[0].events = POLLIN;
    pfds[1].events = POLLIN;
    pfds[2].events = POLLIN;
    pfds[3].events = POLLIN;

    char buffer[BUFFER_SIZE];

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable failed");

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable port failed");

    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    
    server_addr.sin_port = htons(strtol(argv[2], NULL, 10));

    if(connect(server_fd, (sockaddr*)(&server_addr), sizeof(server_addr)))
        perror("FAILED: Connect");

    while(1)
    {
        response_recieved = false;
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        int poll_count = poll(pfds, 4, -1);
        if (poll_count == -1)
            perror("Poll error");

        //here is about player messages
        if(pfds[0].revents & POLLIN)
        {
            int player_bytes = read(STDIN, buffer, BUFFER_SIZE);
            if(player_bytes > 0)
                send(server_fd, buffer, player_bytes, 0);
        }

        //here is about server messages
        if(pfds[1].revents & POLLIN)
        {   
            int server_bytes = recv(server_fd, buffer, BUFFER_SIZE, 0);
            int room_port;
            if((server_bytes > 0) && (sscanf(buffer, "port:%d", &room_port) == 1))
            {
                struct sockaddr_in room_addr;
                int new_socket, opt = 1;

                room_addr.sin_family = AF_INET;
                if(inet_pton(AF_INET, ipaddr, &(room_addr.sin_addr)) == -1)
                    perror("FAILED: Input ipv4 address invalid");

                if((new_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
                    perror("FAILED: Socket was not created");

                if(setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
                    perror("FAILED: Making socket reusable failed");

                if(setsockopt(new_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
                    perror("FAILED: Making socket reusable port failed");

                memset(room_addr.sin_zero, 0, sizeof(room_addr.sin_zero));
                
                room_addr.sin_port = htons(room_port);

                if(connect(new_socket, (sockaddr*)(&room_addr), sizeof(room_addr)) < 0)
                    perror("FAILED: Connect player to room");
                pfds[2].fd = new_socket;
            }
            else if(server_bytes > 0)
            {
                write(STDOUT, buffer, BUFFER_SIZE);
            }
        }

        if(pfds[2].revents & POLLIN)
        {
            int server_bytes = recv(pfds[2].fd, buffer, BUFFER_SIZE, 0);
            //ckeck_for_sending_selection_menu
            if(server_bytes > 0)
            {
                int what_to_do = handle_the_recieve_message(buffer);
                switch (what_to_do)
                {
                case 1:
                {
                    write(1, buffer, BUFFER_SIZE);
                    fd_set file_ds;
                    FD_ZERO(&file_ds);
                    FD_SET(STDIN_FILENO, &file_ds);

                    struct timeval time_out;
                    time_out.tv_sec = 10;
                    time_out.tv_usec = 0;

                    char move[5];
                    memset(move, 0, 5);
                    room_fd = pfds[2].fd;
                    int sel_val = select(STDIN_FILENO + 1, &file_ds, NULL, NULL, &time_out);
                    if(sel_val == 0)
                    {
                        send(server_file_des, "&*", strlen("&*"), 0);
                        send(room_fd, NO_ANSWER.c_str(), NO_ANSWER.size(), 0);
                        continue;
                    }
                    int read_bytes = read(STDIN, move, 1);
                    if(read_bytes > 0)
                    {
                        send(pfds[2].fd, move, strlen(move), 0);
                        send(server_fd, "&*", strlen("&*"), 0);
                    }
                }
                break;
                
                default:
                {
                    write(1, buffer, BUFFER_SIZE);
                }
                break;
                }
                
            }
            
        }

        if(pfds[3].revents & POLLIN)
        {
            int read_bytes = recvfrom(broad_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&broadcast_addr, &broadcast_len);
            if(read_bytes > 0)
            {
                write(STDOUT, buffer, read_bytes);
                if(handle_the_recieve_message(buffer) == 2)
                {
                    exit(0);
                }
            }
        }
    }
}