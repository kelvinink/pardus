#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <string>
#include <cstring>

#include "pd_util.h"
#include "pd_net.h"
#include "pd_http_server.h"

std::string msg = "Hello from server";

void server_iterative();
void server_multiprocess();
void server_multithread();


int main(int argc, char const *argv[]){
    //server_iterative();
    //server_multiprocess();
    server_multithread();
    return 0;
}

//class Task{
//public:
//    Task() = default;
//    Task(SocketChannel sockChan): mSockChan(std::move(sockChan)){}
//    void operator()(){
//        //Process accepted connections
//        std::cout << "Waiting for connection from client" << std::endl;
//        SocketChannel* accChan = new SocketChannel(std::move(sockchan.accept()));
//        std::cout << "New connection from: " << accChan->get_socket().get_remote_addr().to_string() << std::endl;
//
//        pthread_t tid;
//        int err;
//        if((err = pthread_create(&tid, nullptr, connection_processor, accChan)) != 0){
//            std::cerr << "Thread spawn error: " << std::strerror(errno) << std::endl;
//        }
//        pthread_detach(tid);
//        std::cout << "Main thread continue listening from clients" << std::endl;
//    }
//
//private:
//    SocketChannel mSockChan;
//};


void connection_processor(SocketChannel accChan){
    std::cout << "Worker thread processing connection from: " << accChan.get_remote_addr().to_string() << std::endl;

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


void server_multithread(){
    SocketChannel sockchan;
    int server_fd = sockchan.listen(SocketAddress("localhost", SERVER_PORT));
    if(server_fd < 0){
        std::cerr << "Bind to port SERVER_PORT failed: " << std::strerror(errno) << std::endl;
    }

    std::cout << "Is server listening: " << sockchan.is_listening() << std::endl;
    std::cout << "Server address: " << sockchan.get_local_addr().to_string() << std::endl;

    while(1){
        std::cout << "Waiting for connection from client" << std::endl;
        SocketChannel accChan = sockchan.accept();
        std::thread(connection_processor, std::move(accChan)).detach();
        std::cout << "Main thread continue listening from clients" << std::endl;
    }
}


void server_multiprocess(){
    SocketChannel sockchan;
    int server_fd = sockchan.listen(SocketAddress("localhost", SERVER_PORT));
    if(server_fd < 0){
        std::cerr << "Bind to port SERVER_PORT failed: " << std::strerror(errno) << std::endl;
    }

    std::cout << "Is server listening: " << sockchan.is_listening() << std::endl;
    std::cout << "Server address: " << sockchan.get_local_addr().to_string() << std::endl;

    while(1){
        std::cout << "Waiting for connection from client" << std::endl;
        SocketChannel accChan = std::move(sockchan.accept());
        std::cout << "New connection from: " << accChan.get_remote_addr().to_string() << std::endl;

        pid_t pid;
        if((pid = fork()) < 0){
            std::cerr << "Fork error: " << std::strerror(errno) << std::endl;
        }else if(pid == 0){
            std::cout << "Child processing connection from: " << accChan.get_remote_addr().to_string() << std::endl;
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
        }
    }
}



void server_iterative(){
    SocketChannel sockchan;
    int server_fd = sockchan.listen(SocketAddress("localhost", SERVER_PORT));
    if(server_fd < 0){
        std::cerr << "Bind to port SERVER_PORT failed: " << std::strerror(errno) << std::endl;
    }

    std::cout << "Is server listening: " << sockchan.is_listening() << std::endl;
    std::cout << "Server address: " << sockchan.get_local_addr().to_string() << std::endl;

    while(1){
        std::cout << "Waiting for connection from client" << std::endl;
        SocketChannel accChan = std::move(sockchan.accept());
        std::cout << "New connection from: " << accChan.get_remote_addr().to_string() << std::endl;

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