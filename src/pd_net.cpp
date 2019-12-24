#include "pd_net.h"

#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <memory>
#include <iostream>

namespace pardus {
namespace nio {

/***************************
* BytBuffer implementation
**************************/
ByteBuffer::ByteBuffer(){
    ByteBuffer(0);
}

ByteBuffer::ByteBuffer(size_t capacity){
    allocate(capacity);
}

ByteBuffer::ByteBuffer(ByteBuffer&& src){
    mPos = src.mPos;
    mLimit = src.mLimit;
    mCapacity = src.mCapacity;
    mBuff = src.mBuff;

    src.mBuff = nullptr;
    src.mPos = 0;
    src.mLimit = 0;
    src.mCapacity = 0;
}

ByteBuffer::~ByteBuffer(){
    deallocate();
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer&& src){
    deallocate();
    // [Note] Effective cpp item12: copy or move all parts of an object
    mPos = src.mPos;
    mLimit = src.mLimit;
    mCapacity = src.mCapacity;
    mBuff = src.mBuff;

    src.mBuff = nullptr;
    src.mPos = 0;
    src.mLimit = 0;
    src.mCapacity = 0;
}

void ByteBuffer::allocate(size_t capacity){
    deallocate();

    mBuff = new Byte[capacity];
    mPos = 0;
    mLimit = 0;
    mCapacity = capacity;
}

void ByteBuffer::deallocate(){
    delete[](mBuff);

    mBuff = nullptr;
    mPos = 0;
    mLimit = 0;
    mCapacity = 0;
}

// Return raw array of this buffer
Byte* ByteBuffer::array(){
    return mBuff;
}

// Get pos
size_t ByteBuffer::pos() {
    return mPos;
}

// Set pos
void ByteBuffer::pos(size_t newpos) {
    mPos = newpos;
}

// Get mLimit;
size_t ByteBuffer::limit() {
    return mLimit;
}

// Set mLimit
void ByteBuffer::limit(size_t newlimit) {
    mLimit = newlimit;
}

// Return capacity of this buffer
size_t ByteBuffer::capacity() {
    return mCapacity;
}

// Remaining space between mLimit and mPos
size_t ByteBuffer::remaining() {
    return mLimit - mPos;
}

// Return true if there are any space between mLimit and mPos
bool ByteBuffer::hasRemaining() {
    return mPos < mLimit;
}

// Clear buffer for reading in
void ByteBuffer::clear() {
    mPos = 0;
    mLimit = mCapacity;
}

// Flip buffer for writing out
void ByteBuffer::flip() {
    mLimit = mPos;
    mPos = 0;
}

// Reset pos as 0
void ByteBuffer::rewind() {
    mPos = 0;
}

// Return one byte, increase mPos by 1
Byte ByteBuffer::get() {
    if(mPos >= mLimit)
        throw std::out_of_range("Pos out of range");
    return mBuff[mPos++];
}

// Return length bytes to dst. Write start from offset of dst.
// If this->remaining() is larger than length, throw an error.
// mPos increase by min(length, this->remaining())
void ByteBuffer::get(Byte *dst, size_t offset, size_t length) {
    if(length > remaining())
        throw std::length_error("Not enough of remaining items");
    for(int i = offset; i < offset+length; i++)
        dst[i] = get();
}

// Return byte at index, mPos not changed
Byte ByteBuffer::get(size_t index) {
    if(index >= mLimit)
        throw std::out_of_range("Index out of range");
    return mBuff[index];
}

// Return one char, increase pos by sizeof(char)==1
char ByteBuffer::getchar() {
    return static_cast<char>(get());
}

// Return one char at index, mPos is not changed
char ByteBuffer::getchar(size_t index) {
    return static_cast<char>(get(index));
}

// Put one byte to buffer, increase mPos by 1
void ByteBuffer::put(Byte b) {
    if(mPos >= mLimit)
        throw std::range_error("Invalid pos");

    *(mBuff + mPos) = b;
    mPos++;
}

// Put at most length bytes from src to buffer start from offset.
// If length > this->remaining(), throw an error.
// mPos increase by min(length, this->remaining())
void ByteBuffer::put(Byte *src, size_t offset, size_t length) {
    if(length > remaining())
        throw std::range_error("Not enough of remaining space");
    for(int i = offset; i < offset+length; i++)
        put(src[i]);
}

// Put one byte at index, mPos is not changed.
void ByteBuffer::put(size_t index, Byte b) {
    if(index >= mLimit)
        throw std::range_error("Index range error");
    mBuff[index] = b;
}

// Transfer data from src to this buffer, all or nothing
void ByteBuffer::put(ByteBuffer &src) {
    if(src.remaining() > remaining())
        throw std::range_error("Not enough of remaining space");
    while(src.hasRemaining()){
        put(src.get());
    }
}

// Put one char to this buffer
void ByteBuffer::putchar(char val) {
    put(static_cast<Byte>(val));
}

// Put one char at index
void ByteBuffer::putchar(size_t index, char val) {
    put(index, static_cast<Byte>(val));
}

// Convert all data in buffer to a string
// Buffer becomes empty
std::string ByteBuffer::toString() {
    std::string ret;
    while(hasRemaining())
        ret.push_back(static_cast<char>(get()));
    return ret;
}


/******************************
* SocketAddress implementation
******************************/
// Constructing SocketAddress from sockaddr
// This is useful when accepting a socket connection
SocketAddress SocketAddress::fromSockaddr(sockaddr* addr, int length){
    char hostbuff[NI_MAXHOST];
    char servbuff[NI_MAXSERV];
    getnameinfo(addr, length, hostbuff, NI_MAXHOST,
                servbuff, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);

    return SocketAddress(std::string(hostbuff), std::stoi(std::string(servbuff)));
}


/************************
* Socket implementation
***********************/
Socket::Socket(){
    //[Note] Effective cpp item4: Make sure that objects are initialized before they are used
    clear();
}

Socket::Socket(Socket&& rhs){
    operator=(std::forward<Socket>(rhs));
}

Socket& Socket::operator=(Socket&& rhs){
    mSocketFd = rhs.mSocketFd;
    mLocalAddr = rhs.mLocalAddr;
    mRemoteAddr = rhs.mRemoteAddr;
    mStatus = rhs.mStatus;
    rhs.clear();
}

Socket::~Socket(){
    // [Note] Effective cpp Item8: Prevent exceptions from leaving destructors
    // All exceptions should have been catched and processed.
    try{
        this->close();
    }catch(std::exception& e){
        std::cerr << "Destructing socket failed: " << e.what() << std::endl;
    }
}

void Socket::clear(){
    mSocketFd = -1;
    mLocalAddr = SocketAddress();
    mRemoteAddr = SocketAddress();
    mStatus = Status::PD_SOCK_UNBOUND;
}

// listen - Open and return a listening socket on port.
// This function is reentrant and protocol-independent.
//     Return listen socket discriptor
//     On error, returns -1 and sets errno.
int Socket::listen(const SocketAddress &bindpoint) {
    addrinfo hints, *listp, *p;
    int listenfd, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Accept TCP connections */
    hints.ai_flags = AI_PASSIVE;      /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV; /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    getaddrinfo(NULL, std::to_string(bindpoint.mPort).c_str(), &hints, &listp);

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int));

        if (::bind(listenfd, p->ai_addr, p->ai_addrlen) == 0){
            break; /* Success */
        }
        ::close(listenfd);
    }

    freeaddrinfo(listp);
    if (!p)
        return -1;

    if (::listen(listenfd, LISTENQ) < 0){
        return -1;
    }else{
        mSocketFd = listenfd;
        mLocalAddr = bindpoint;
        mStatus = Status::PD_SOCK_LISTENING;
        return listenfd;
    }
}

// Connect - Connecting to a remote server
//     Return socket connect file discriptor
//     One error, return -1
int Socket::connect(const SocketAddress &endpoint) {
    int connectfd;
    addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV;  /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    getaddrinfo(endpoint.mHost.c_str(), std::to_string(endpoint.mPort).c_str(), &hints, &listp);

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p != nullptr; p = p->ai_next) {
        if ((connectfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;
        if (::connect(connectfd, p->ai_addr, p->ai_addrlen) != -1){
            break; /* Success */
        }
        ::close(connectfd);
    }

    freeaddrinfo(listp);
    if (!p){
        return -1;
    }
    else{
        mSocketFd = connectfd;
        mRemoteAddr = endpoint;
        mStatus = Status::PD_SOCK_CONNECTED;
        return connectfd;
    }
}

// Accept - Accepting a new connection
//     Return a new Socket
//     One error, exit
Socket Socket::accept(){
    if(!(getStatus() == Socket::Status::PD_SOCK_LISTENING))
        throw std::runtime_error("The server is not listening");
    int cnxxfd;
    sockaddr_in clientaddr;
    int clientlen = sizeof(clientaddr);
    if ((cnxxfd = ::accept(mSocketFd, (struct sockaddr *)&clientaddr, (socklen_t*)&clientlen)) < 0){
        std::cerr << "In accept failed: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    Socket accSocket;
    accSocket.mSocketFd = cnxxfd;
    accSocket.mStatus = Socket::Status::PD_SOCK_ACCEPTED;
    accSocket.mLocalAddr = getLocalAddr();
    accSocket.mRemoteAddr = SocketAddress::fromSockaddr((struct sockaddr *) &clientaddr, clientlen);
    return std::move(accSocket);
}

void Socket::close() {
    if(mSocketFd >= 0){
        if(::close(mSocketFd) < 0)
            throw std::runtime_error("Socket close failed");
        clear();
    }
    mStatus = Status::PD_SOCK_CLOSED;
}

SocketAddress Socket::getLocalAddr() {
    return mLocalAddr;
}

SocketAddress Socket::getRemoteAddr() {
    return mRemoteAddr;
}

int Socket::getStatus(){
    return mStatus;
}

int Socket::getSocketFd(){
    return mSocketFd;
}

/******************************
* SocketChannel implementation
******************************/
SocketChannel::SocketChannel(){
    SocketChannel(Socket());
}

SocketChannel::SocketChannel(Socket socket) {
    mSocket = std::move(socket);
    mRbuff.allocate(BUFFSIZE);
}

// Listening at local.mPort
//    Return listening socket file discriptor
int SocketChannel::listen(const SocketAddress& local) {
    return mSocket.listen(local);
}

// Connect to remote server
//    Return connect socket file discriptor
int SocketChannel::connect(const SocketAddress& remote) {
    return mSocket.connect(remote);
}

// Accept a new socket connection
//    Return a new SocketChannel that's accepted
SocketChannel SocketChannel::accept() {
    return std::move(SocketChannel(std::move(mSocket.accept())));
}

// Read from channel to dst
// Buffered read
//    Return number of bytes transfered
//    Return 0 when there is EOF
//    On error, return -1
ssize_t SocketChannel::read(ByteBuffer &dst) {
    // Refill mRbuff if it's empty
    while(!mRbuff.hasRemaining()){
        size_t n = mRbuff.capacity();
        std::unique_ptr<Byte[]> tmp(new Byte[n]);
        ssize_t nread = ::read(mSocket.getSocketFd(), (void*)tmp.get(), n);
        if(nread < 0){
            return -1;
        }else if(nread == 0){
            return 0; // EOF
        }

        // Preparing for writing from mRbuff to dst
        mRbuff.clear();
        mRbuff.put(tmp.get(), 0, nread);
        mRbuff.flip();
    }

    ssize_t count = 0;
    while(mRbuff.hasRemaining() && dst.hasRemaining()){
        dst.put(mRbuff.get());
        count++;
    }
    return count;
}

// Write to network at best effort,
// Not sure how many bytes to be written
//    Return number of bytes sent to network
//    On error, return -1
// Todo: this function invaded Bytebuffer raw array.
ssize_t SocketChannel::write(ByteBuffer &src) {
    ssize_t count = 0;
    if(src.hasRemaining()){
        // Consuming src, it's not thread safe
        ssize_t nwrite = ::write(mSocket.getSocketFd(), src.array() + src.pos(), src.remaining());
        if(nwrite < 0){
            return -1;
        }
        count += nwrite;
        src.pos(src.pos()+nwrite);
    }
    return count;
}

void SocketChannel::close() {
    mSocket.close();
}

bool SocketChannel::isOpen() {
    return !(mSocket.getStatus() == Socket::Status::PD_SOCK_CLOSED);
}

bool SocketChannel::isListening() {
    return getStatus() == Socket::Status::PD_SOCK_LISTENING;
}

bool SocketChannel::isConnected() {
    return getStatus() == Socket::Status::PD_SOCK_CONNECTED;
}

bool SocketChannel::isAccepted() {
    return getStatus() == Socket::Status::PD_SOCK_ACCEPTED;
}

bool SocketChannel::isClosed() {
    return getStatus() == Socket::Status::PD_SOCK_CLOSED;
}

int SocketChannel::getStatus(){
    return mSocket.getStatus();
}

SocketAddress SocketChannel::getLocalAddr() {
    return mSocket.getLocalAddr();
}

SocketAddress SocketChannel::getRemoteAddr() {
    return mSocket.getRemoteAddr();
}

} // namespace nio
} // namespace pardus
















