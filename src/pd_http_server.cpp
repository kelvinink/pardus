#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <netinet/in.h>
#include <string>

#include "pd_util.h"
#include "pd_net.h"
#include "pd_http_server.h"

#define MAXLINE 3000

std::string msg = "Hello from server";

int main(int argc, char const *argv[]){
    SocketChannel sockchan;
    int server_fd = sockchan.listen(SocketAddress("localhost", SERVER_PORT));
    if(server_fd < 0){
        perror("Bind to port 8008 failed");
    }
    std::cout << "Socket status: " << sockchan.get_socket().get_status() << std::endl;
    std::cout << "local addr: " << sockchan.get_socket().get_local_addr().to_string() << std::endl;


    while(1){
        printf("\nWaiting for new connection\n");

        SocketChannel accChan = std::move(sockchan.accept());
        ByteBuffer buffer(BUFFSIZE);
        buffer.clear();
        accChan.read(buffer);
        buffer.flip();
        std::cout << buffer.to_string() << std::endl;

        buffer.clear();
        buffer.put((Byte*)msg.c_str(), 0, msg.size());
        buffer.flip();
        accChan.write(buffer);

        accChan.close();
    }
    return 0;
}