#ifndef PD_NET_H
#define PD_NET_H

#include <string>
#include <algorithm>
#include "pd_types.h"

#define LISTENQ 1024
#define BUFFSIZE 8092
#define SERVER_PORT 8008

int pd_connect(const std::string& hostname, const std::string& port);
int pd_listen(const std::string& port);

/*ByteBuffer - Buffer of bytes
 */
class ByteBuffer{
public:
    ByteBuffer();
    ByteBuffer(size_t capacity);
    ByteBuffer(const ByteBuffer& rhs) = delete;
    ByteBuffer& operator=(const ByteBuffer& rhs) = delete;
    ByteBuffer(ByteBuffer&& src);
    ~ByteBuffer();

    void allocate(size_t capacity);
    void deallocate();
    Byte* array();
    size_t pos();
    void pos(size_t newpos);
    size_t limit();
    void limit(size_t newlimit);
    size_t capacity();
    size_t remaining();
    bool has_remaining();

    void clear();
    void flip();
    void rewind();

    Byte get();
    void get(Byte* dst, size_t offset, size_t length);
    Byte get(size_t index);
    char getchar();
    char getchar(size_t index);
    void put(Byte b);
    void put(Byte* src, size_t offset, size_t length);
    void put(size_t index, Byte b);
    void put(ByteBuffer &src);
    void putchar(char val);
    void putchar(size_t index, char val);
    std::string to_string();

private:
    size_t mpos = 0;
    size_t mlimit = 0;
    size_t mcapacity = 0;
    Byte* mbuff = nullptr;

    void _incpos(size_t n);
};


/* Channel - NIO channel
 */
class Channel{
public:
    virtual void close() = 0;
    virtual bool is_open() = 0;
};


/* SocketAddress
 */
struct SocketAddress{
    std::string mhost;
    int  mport;
    SocketAddress(){mhost = ""; mport = -1;};
    SocketAddress(const std::string& host, int port):mhost(host),mport(port){}
    std::string to_string(){return mhost + " " + std::to_string(mport);}
};


/* Socket
 */
class Socket{
public:
    Socket();
    int listen(const SocketAddress& bindpoint);
    int connect(const SocketAddress& endpoint);
    //int connect(const SocketAddress& endpoint, int timeout);
    void close();
    bool islistening();
    bool isconnected();
    bool isaccepted();
    bool isclosed();
    SocketAddress get_local_addr();
    SocketAddress get_remote_addr();
    void set_local_addr(const SocketAddress& local);
    void set_remote_addr(const SocketAddress& remote);
    int get_status();
    void set_status(int status);
    int get_socketfd();
    void set_socketfd(int socketfd);

public:
    enum SocketStatus{
        PD_SOCK_UNBOUND,
        PD_SOCK_LISTENING,
        PD_SOCK_CONNECTED,
        PD_SOCK_ACCEPTED,
        PD_SOCK_CLOSED
    };

private:
    SocketAddress mlocaladdr;
    SocketAddress mremoteaddr;
    int mstatus;
    int msocketfd;
};


/*SocketChannel - Channel of socket
 */
class SocketChannel: public Channel{
public:
    SocketChannel();
    SocketChannel(Socket socket);

    int listen(SocketAddress local);
    int connect(SocketAddress remote);
    SocketChannel accept();
    ssize_t read(ByteBuffer &dst);
    ssize_t write(ByteBuffer &src);
    Socket get_socket();
    void close() override ;
    bool is_open() override;

private:
    Socket msocket;
    ByteBuffer mwbuff;
    ByteBuffer mrbuff;
};

#endif //PD_NET_H