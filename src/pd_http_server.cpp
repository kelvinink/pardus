#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string>

#include "pd_net.h"
#include "pd_http_server.h"

#define MAXLINE 3000
std::string msg = "Hello from server";

int main(int argc, char const *argv[]){
    int server_fd;
    if((server_fd = pd_listen("8008")) < 0){
        perror("Bind to port 8008 failed");
    }

    while(1){
        printf("\nWaiting for new connection\n");

        int cnxxfd;
        sockaddr_in clientaddr;
        int clientlen = sizeof(clientaddr);
        if ((cnxxfd = accept(server_fd, (struct sockaddr *)&clientaddr, (socklen_t*)&clientlen)) < 0){
            perror("In accept failed");
            exit(EXIT_FAILURE);
        }
        
        char buffer[MAXLINE] = {0};
        long valread = read( cnxxfd , buffer, MAXLINE);
        std::cout << buffer << std::endl;
        write(cnxxfd , msg.c_str() , msg.size());
        std::cout << "Hello message sent" << std::endl;
        close(cnxxfd);
    }
    return 0;
}