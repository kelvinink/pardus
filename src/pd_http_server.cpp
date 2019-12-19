#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <netinet/in.h>
#include <string>

#include "pd_util.h"
#include "pd_net.h"
#include "pd_http_server.h"

#define MAXLINE 8192

std::string msg = "Hello from server";

void server_iterative();
void server_multiprocess();


int main(int argc, char const *argv[]){
    //server_iterative();
    server_multiprocess();
    return 0;
}


void server_multiprocess(){
    SocketChannel sockchan;
    int server_fd = sockchan.listen(SocketAddress("localhost", SERVER_PORT));
    if(server_fd < 0){
        perror("Bind to port 8008 failed");
    }

    std::cout << "Is server listening: " << sockchan.get_socket().islistening() << std::endl;
    std::cout << "Server address: " << sockchan.get_socket().get_local_addr().to_string() << std::endl;

    while(1){
        std::cout << "Waiting for connection from client" << std::endl;
        SocketChannel accChan = std::move(sockchan.accept());
        std::cout << "New connection from: " << accChan.get_socket().get_remote_addr().to_string() << std::endl;

        pid_t pid;
        if((pid = fork()) < 0){
            perror("Fork error");
        }else if(pid == 0){
            std::cout << "Child processing connection from: " << accChan.get_socket().get_remote_addr().to_string() << std::endl;
            // Read from channel
            ByteBuffer buffer(BUFFSIZE);
            buffer.clear();
            accChan.read(buffer);
            buffer.flip();
            std::cout << buffer.to_string() << std::endl;

            // Write to channel
            buffer.clear();
            buffer.put((Byte*)msg.c_str(), 0, msg.size());
            buffer.flip();
            accChan.write(buffer);

            accChan.close();
            exit(0);
        }else{
            std::cout << "Child pid is:" << pid << std::endl;
            //do nothing continue listening
        }
    }
}


void server_iterative(){
    SocketChannel sockchan;
    int server_fd = sockchan.listen(SocketAddress("localhost", SERVER_PORT));
    if(server_fd < 0){
        perror("Bind to port 8008 failed");
    }

    std::cout << "Is server listening: " << sockchan.get_socket().islistening() << std::endl;
    std::cout << "Server address: " << sockchan.get_socket().get_local_addr().to_string() << std::endl;

    while(1){
        std::cout << "Waiting for connection from client" << std::endl;
        SocketChannel accChan = std::move(sockchan.accept());
        std::cout << "New connection from: " << accChan.get_socket().get_remote_addr().to_string() << std::endl;

        // Read from channel
        ByteBuffer buffer(BUFFSIZE);
        buffer.clear();
        accChan.read(buffer);
        buffer.flip();
        std::cout << buffer.to_string() << std::endl;

        // Write to channel
        buffer.clear();
        buffer.put((Byte*)msg.c_str(), 0, msg.size());
        buffer.flip();
        accChan.write(buffer);

        accChan.close();
    }
}