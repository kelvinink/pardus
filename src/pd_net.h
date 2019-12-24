#ifndef PD_NET_H
#define PD_NET_H

#include <sys/socket.h>
#include <string>
#include <algorithm>

#include "pd_types.h"

#define LISTENQ 1024
#define BUFFSIZE 8192
#define SERVER_PORT 8008

namespace pardus {
namespace nio {

class ByteBuffer;
class Socket;
class SocketAddress;
class Channel;
class SocketChannel;


// ByteBuffer - Buffer of bytes
class ByteBuffer {
public:
    ByteBuffer();
    explicit ByteBuffer(size_t capacity);
    ByteBuffer(const ByteBuffer &) = delete;
    ByteBuffer& operator=(const ByteBuffer &) = delete;
    ByteBuffer(ByteBuffer &&src);
    ByteBuffer& operator=(ByteBuffer &&src);
    ~ByteBuffer();

    void allocate(size_t capacity);
    void deallocate();

    size_t pos();
    void pos(size_t newpos);
    size_t limit();
    void limit(size_t newlimit);
    size_t capacity();
    size_t remaining();
    bool hasRemaining();

    void clear();
    void flip();
    void rewind();

    Byte *array();
    Byte get();
    void get(Byte *dst, size_t offset, size_t length);
    Byte get(size_t index);
    char getchar();
    char getchar(size_t index);
    void put(Byte b);
    void put(Byte *src, size_t offset, size_t length);
    void put(size_t index, Byte b);
    void put(ByteBuffer &src);
    void putchar(char val);
    void putchar(size_t index, char val);
    std::string toString();

private:
    size_t mPos = 0;
    size_t mLimit = 0;
    size_t mCapacity = 0;
    Byte *mBuff = nullptr;
};


// Channel - NIO channel
class Channel {
public:
    //[Note] Effective cpp item7: Declare destructors virtual in polymorphic base classes
    virtual ~Channel() {}
    virtual void close() = 0;
    virtual bool isOpen() = 0;
};

// SocketAddress
class SocketAddress {
public:
    SocketAddress() :mHost(""), mPort(-1){}
    SocketAddress(const std::string &host, int port) : mHost(host), mPort(port) {}
    static SocketAddress fromSockaddr(::sockaddr *addr, int length);
    std::string toString() { return mHost + " " + std::to_string(mPort); }

public:
    std::string mHost;
    int mPort;
};


// Socket
class Socket {
public:
    Socket();
    Socket(const Socket &) = delete;
    Socket(Socket&& rhs);
    Socket& operator=(const Socket &) = delete;
    Socket& operator=(Socket &&);
    ~Socket();


    int listen(const SocketAddress &bindpoint);
    int connect(const SocketAddress &endpoint);
    //int connect(const SocketAddress& endpoint, int timeout);
    Socket accept();
    void close();

    int getStatus();
    int getSocketFd();
    SocketAddress getLocalAddr();
    SocketAddress getRemoteAddr();

public:
    // [Note] Effective cpp item2: Prefer consts, enums, and inlines to #defines
    enum Status {
        PD_SOCK_UNBOUND,
        PD_SOCK_LISTENING,
        PD_SOCK_CONNECTED,
        PD_SOCK_ACCEPTED,
        PD_SOCK_CLOSED
    };

private:
    void clear();

private:
    SocketAddress mLocalAddr;
    SocketAddress mRemoteAddr;
    int mStatus;
    int mSocketFd;
};


//SocketChannel - Channel of socket
class SocketChannel : public Channel {
public:
    SocketChannel();
    SocketChannel(Socket socket);

    int listen(const SocketAddress &local);
    int connect(const SocketAddress &remote);
    SocketChannel accept();
    void close() override;

    ssize_t read(ByteBuffer &dst);
    ssize_t write(ByteBuffer &src);

    bool isOpen() override;
    bool isListening();
    bool isConnected();
    bool isAccepted();
    bool isClosed();
    int getStatus();
    SocketAddress getLocalAddr();
    SocketAddress getRemoteAddr();

private:
    Socket mSocket;
    ByteBuffer mRbuff;
};

} // namespace nio
} // namespace pardus


#endif //PD_NET_H