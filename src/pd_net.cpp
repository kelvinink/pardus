#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <memory>

#include "pd_net.h"

/***************************************************
 * BytBuffer implementation
 **************************************************/
ByteBuffer::ByteBuffer(){
    ByteBuffer(0);
}

ByteBuffer::ByteBuffer(size_t capacity){
    allocate(capacity);
}

ByteBuffer::ByteBuffer(ByteBuffer&& src){
    mpos = src.mpos;
    mlimit = src.mlimit;
    mcapacity = src.mcapacity;
    mbuff = src.mbuff;
    src.mbuff = nullptr;
    src.mpos = 0;
    src.mlimit = 0;
    src.mcapacity = 0;
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer&& src){
    deallocate();
    mpos = src.mpos;
    mlimit = src.mlimit;
    mcapacity = src.mcapacity;
    mbuff = src.mbuff;
    src.mbuff = nullptr;
    src.mpos = 0;
    src.mlimit = 0;
    src.mcapacity = 0;
}

void ByteBuffer::allocate(size_t capacity){
    //Todo: to strengthen memory management
    deallocate();
    mbuff = new Byte[capacity];
    mpos = 0;
    mlimit = 0;
    mcapacity = capacity;
}

void ByteBuffer::deallocate(){
    //Todo: to strengthen memory management
    delete[](mbuff);
    mbuff = nullptr;
    mpos = 0;
    mlimit = 0;
    mcapacity = 0;
}

ByteBuffer::~ByteBuffer(){
    deallocate();
}

// Return raw array of this buffer
Byte* ByteBuffer::array(){
    return mbuff;
}

// Get pos
size_t ByteBuffer::pos() {
    return mpos;
}

// Set pos
void ByteBuffer::pos(size_t newpos) {
    mpos = newpos;
}

// Get mlimit;
size_t ByteBuffer::limit() {
    return mlimit;
}

// Set mlimit
void ByteBuffer::limit(size_t newlimit) {
    mlimit = newlimit;
}

// Return capacity of this buffer
size_t ByteBuffer::capacity() {
    return mcapacity;
}

// Remaining space between mlimit and mpos
size_t ByteBuffer::remaining() {
    return mlimit-mpos;
}

// Return true if there are any space between mlimit and mpos
bool ByteBuffer::has_remaining() {
    return mpos < mlimit;
}

// Clear buffer for reading in
void ByteBuffer::clear() {
    mpos = 0;
    mlimit = mcapacity;
}

// Flip buffer for writing out
void ByteBuffer::flip() {
    mlimit = mpos;
    mpos = 0;
}

// Reset pos as 0
void ByteBuffer::rewind() {
    mpos = 0;
}

// Return one byte, increase mpos by 1
Byte ByteBuffer::get() {
    if(mpos >= mlimit)
        throw std::out_of_range("Pos out of range");
    return mbuff[mpos++];
}

// Return length bytes to dst. Write start from offset of dst.
// If this->remaining() is larger than length, throw an error.
// mpos increase by min(length, this->remaining())
void ByteBuffer::get(Byte *dst, size_t offset, size_t length) {
    if(length > remaining())
        throw std::length_error("Not enough of remaining items");
    for(int i = offset; i < offset+length; i++)
        dst[i] = get();
}

// Return byte at index, mpos not changed
Byte ByteBuffer::get(size_t index) {
    if(index >= mlimit)
        throw std::out_of_range("Index out of range");
    return mbuff[index];
}

// Return one char, increase pos by sizeof(char)==1
char ByteBuffer::getchar() {
    return static_cast<char>(get());
}

// Return one char at index, mpos is not changed
char ByteBuffer::getchar(size_t index) {
    return static_cast<char>(get(index));
}

// Put one byte to buffer, increase mpos by 1
void ByteBuffer::put(Byte b) {
    if(mpos >= mlimit)
        throw std::range_error("Invalid pos");

    *(mbuff+mpos) = b;
    mpos++;
}

// Helper function, increase mpos by n
void ByteBuffer::_incpos(size_t n) {
    mpos += n;
}

// Put at most length bytes from src to buffer start from offset.
// If length > this->remaining(), throw an error.
// mpos increase by min(length, this->remaining())
void ByteBuffer::put(Byte *src, size_t offset, size_t length) {
    if(length > remaining())
        throw std::range_error("Not enough of remaining space");
    for(int i = offset; i < offset+length; i++)
        put(src[i]);
}

// Put one byte at index, mpos is not changed.
void ByteBuffer::put(size_t index, Byte b) {
    if(index >= mlimit)
        throw std::range_error("Index range error");
    mbuff[index] = b;
}

// Transfer data from src to this buffer, all or nothing
void ByteBuffer::put(ByteBuffer &src) {
    if(src.remaining() > remaining())
        throw std::range_error("Not enough of remaining space");
    while(src.has_remaining()){
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
std::string ByteBuffer::to_string() {
    std::string ret;
    while(has_remaining())
        ret.push_back(static_cast<char>(get()));
    return ret;
}


/***************************************************
 * SocketAddress implementation
 **************************************************/
 // Constructing SocketAddress from sockaddr
 // This is useful when accepting a socket connection
SocketAddress SocketAddress::from_sockaddr(sockaddr* addr, int length){
    char hostbuff[NI_MAXHOST];
    char servbuff[NI_MAXSERV];
    getnameinfo(addr, length, hostbuff, NI_MAXHOST,
                servbuff, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);

    return SocketAddress(std::string(hostbuff), std::stoi(std::string(servbuff)));
}


/***************************************************
 * Socket implementation
 **************************************************/
Socket::Socket() {
    mstatus = SocketStatus::PD_SOCK_UNBOUND;
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
    getaddrinfo(NULL, std::to_string(bindpoint.mport).c_str(), &hints, &listp);

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
        msocketfd = listenfd;
        mlocaladdr = bindpoint;
        mstatus = SocketStatus::PD_SOCK_LISTENING;
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
    getaddrinfo(endpoint.mhost.c_str(), std::to_string(endpoint.mport).c_str(), &hints, &listp);

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
        msocketfd = connectfd;
        mremoteaddr = endpoint;
        mstatus = SocketStatus::PD_SOCK_CONNECTED;
        return connectfd;
    }
}

// Accept - Accepting a new connection
//     Return a new Socket
//     One error, exit
Socket Socket::accept(){
    if(!islistening())
        throw std::runtime_error("The server is not listening");
    int cnxxfd;
    sockaddr_in clientaddr;
    int clientlen = sizeof(clientaddr);
    if ((cnxxfd = ::accept(msocketfd, (struct sockaddr *)&clientaddr, (socklen_t*)&clientlen)) < 0){
        perror("In accept failed");
        exit(EXIT_FAILURE);
    }

    Socket accSocket;
    accSocket.set_socketfd(cnxxfd);
    accSocket.set_status(Socket::SocketStatus::PD_SOCK_ACCEPTED);
    accSocket.set_local_addr(get_local_addr());
    accSocket.set_remote_addr(SocketAddress::from_sockaddr((struct sockaddr *)&clientaddr, clientlen));
    return std::move(accSocket);
}

void Socket::close() {
    ::close(msocketfd);
    mstatus = SocketStatus::PD_SOCK_CLOSED;
}

SocketAddress Socket::get_local_addr() {
    return mlocaladdr;
}

SocketAddress Socket::get_remote_addr() {
    return mremoteaddr;
}

void Socket::set_local_addr(const SocketAddress& local){
    mlocaladdr = local;
}
void Socket::set_remote_addr(const SocketAddress& remote){
    mremoteaddr = remote;
}

int Socket::get_status(){
    return mstatus;
}

void Socket::set_status(int status){
    mstatus = status;
}

int Socket::get_socketfd(){
    return msocketfd;
}

void Socket::set_socketfd(int socketfd){
    msocketfd = socketfd;
}

bool Socket::islistening(){
    return mstatus == SocketStatus::PD_SOCK_LISTENING;
}

bool Socket::isconnected() {
    return mstatus == SocketStatus::PD_SOCK_CONNECTED;
}

bool Socket::isclosed() {
    return mstatus == SocketStatus::PD_SOCK_CLOSED;
}

bool Socket::isaccepted(){
    return mstatus == SocketStatus ::PD_SOCK_ACCEPTED;
}


/***************************************************
 * SocketChannel implementation
 **************************************************/
SocketChannel::SocketChannel(){
    SocketChannel(Socket());
}

SocketChannel::SocketChannel(Socket socket) {
    msocket = socket;
    mrbuff.allocate(BUFFSIZE);
}

// Listening at local.mport
//    Return listening socket file discriptor
int SocketChannel::listen(const SocketAddress& local) {
    return msocket.listen(local);
}

// Connect to remote server
//    Return connect socket file discriptor
int SocketChannel::connect(const SocketAddress& remote) {
    return msocket.connect(remote);
}

// Accept a new socket connection
//    Return a new SocketChannel that's accepted
SocketChannel SocketChannel::accept() {
    Socket accSocket = msocket.accept();
    return std::move(SocketChannel(accSocket));
}

// Read from channel to dst
// Buffered read
//    Return number of bytes transfered
//    Return 0 when there is EOF
//    On error, return -1
ssize_t SocketChannel::read(ByteBuffer &dst) {
    // Refill mrbuff if it's empty
    while(!mrbuff.has_remaining()){
        size_t n = mrbuff.capacity();
        std::unique_ptr<Byte[]> tmp(new Byte[n]);
        ssize_t nread = ::read(msocket.get_socketfd(), (void*)tmp.get(), n);
        if(nread < 0){
            return -1;
        }else if(nread == 0){
            return 0; // EOF
        }

        // Preparing for writing from mrbuff to dst
        mrbuff.clear();
        mrbuff.put(tmp.get(), 0, nread);
        mrbuff.flip();
    }

    ssize_t count = 0;
    while(mrbuff.has_remaining() && dst.has_remaining()){
        dst.put(mrbuff.get());
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
    if(src.has_remaining()){
        // Consuming src, it's not thread safe
        ssize_t nwrite = ::write(msocket.get_socketfd(), src.array()+src.pos(), src.remaining());
        if(nwrite < 0){
            return -1;
        }

        count += nwrite;
        src.pos(src.pos()+nwrite);
    }
    return count;
}

Socket SocketChannel::get_socket() {
    return msocket;
}

void SocketChannel::close() {
    msocket.close();
}

bool SocketChannel::is_open() {
    return !msocket.isclosed();
}











