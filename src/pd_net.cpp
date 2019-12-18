#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <vector>

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

Byte* ByteBuffer::array(){
    return mbuff;
}

size_t ByteBuffer::pos() {
    return mpos;
}

void ByteBuffer::pos(size_t newpos) {
    mpos = newpos;
}

size_t ByteBuffer::limit() {
    return mlimit;
}

void ByteBuffer::limit(size_t newlimit) {
    mlimit = newlimit;
}

size_t ByteBuffer::capacity() {
    return mcapacity;
}

size_t ByteBuffer::remaining() {
    return limit()-pos();
}

bool ByteBuffer::has_remaining() {
    return pos() < limit();
}

void ByteBuffer::clear() {
    pos(0);
    limit(capacity());
}

void ByteBuffer::flip() {
    limit(pos());
    pos(0);
}

void ByteBuffer::rewind() {
    pos(0);
}

Byte ByteBuffer::get() {
    if(pos() >= limit())
        throw std::out_of_range("Invalid pos");
    Byte ret = mbuff[pos()];
    _incpos(1);
    return ret;
}

void ByteBuffer::get(Byte *dst, size_t offset, size_t length) {
    if(length > remaining())
        throw std::length_error("Length is too large");
    for(int i = offset; i < offset+length; i++)
        dst[i] = get();
}

Byte ByteBuffer::get(size_t index) {
    if(index >= limit())
        throw std::out_of_range("Invalid index");
    return mbuff[index];
}

char ByteBuffer::getchar() {
    return static_cast<char>(get());
}

char ByteBuffer::getchar(size_t index) {
    return static_cast<char>(get(index));
}

void ByteBuffer::put(Byte b) {
    if(pos() >= limit())
        throw std::range_error("Invalid pos");

    *(mbuff+pos()) = b;
    _incpos(1);
}


void ByteBuffer::_incpos(size_t n) {
    pos(pos()+n);
}

void ByteBuffer::put(Byte *src, size_t offset, size_t length) {
    if(length > remaining())
        throw std::range_error("Length too large");
    for(int i = offset; i < offset+length; i++)
        put(src[i]);
}

void ByteBuffer::put(size_t index, Byte b) {
    if(index >= limit())
        throw std::range_error("Index range error");
    mbuff[index] = b;
}

void ByteBuffer::put(ByteBuffer &src) {
    if(src.remaining() > remaining())
        throw std::range_error("Too many items from src");
    while(src.has_remaining()){
        put(src.get());
    }
}

void ByteBuffer::putchar(char val) {
    put(static_cast<Byte>(val));
}

void ByteBuffer::putchar(size_t index, char val) {
    put(index, static_cast<Byte>(val));
}

std::string ByteBuffer::to_string() {
    std::string ret;
    while(has_remaining())
        ret.push_back(static_cast<char>(get()));
    return ret;
}


/***************************************************
 * Socket implementation
 **************************************************/
Socket::Socket() {
    mstatus = SocketStatus::PD_SOCK_UNBOUND;
}

/*
* listen - Open and return a listening socket on port.
* This function is reentrant and protocol-independent.
*
*     On error, returns -1 and sets errno.
*/
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
    mwbuff.allocate(BUFFSIZE);
    mrbuff.allocate(BUFFSIZE);
}

int SocketChannel::listen(SocketAddress local) {
    return msocket.listen(local);
}

int SocketChannel::connect(SocketAddress remote) {
    return msocket.connect(remote);
}

SocketChannel SocketChannel::accept() {
    int cnxxfd;
    sockaddr_in clientaddr;
    int clientlen = sizeof(clientaddr);
    if ((cnxxfd = ::accept(msocket.get_socketfd(), (struct sockaddr *)&clientaddr, (socklen_t*)&clientlen)) < 0){
        perror("In accept failed");
        exit(EXIT_FAILURE);
    }

    char hostbuff[NI_MAXHOST];
    char servbuff[NI_MAXSERV];
    getnameinfo((struct sockaddr *)&clientaddr, clientlen, hostbuff, NI_MAXHOST,
            servbuff, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);
    std::string clienthost;
    std::string clientport;
    for(int i = 0; i < NI_MAXHOST && hostbuff[i] != '\0'; i++)
        clienthost.push_back(hostbuff[i]);
    for(int i = 0; i < NI_MAXSERV && servbuff[i] != '\0'; i++)
        clientport.push_back(servbuff[i]);

    Socket accSocket;
    accSocket.set_socketfd(cnxxfd);
    accSocket.set_status(Socket::SocketStatus::PD_SOCK_ACCEPTED);
    accSocket.set_local_addr(SocketAddress("localhost", SERVER_PORT));
    accSocket.set_remote_addr(SocketAddress(clienthost, std::stoi(clientport)));
    SocketChannel accChan = SocketChannel(accSocket);
    return std::move(accChan);
}

ssize_t SocketChannel::read(ByteBuffer &dst) {
    // Refill mrbuff if it's empty
    while(!mrbuff.has_remaining()){
        // Todo: replace Byte array with smart pointer
        size_t n = mrbuff.capacity();
        Byte* tmp = new Byte[n];
        ssize_t nread = ::read(msocket.get_socketfd(), (void*)tmp, n);
        if(nread < 0){
            return -1;
        }else if(nread == 0){
            return 0; // EOF
        }

        // Preparing for writing from mrbuff to dst
        mrbuff.clear();
        mrbuff.put(tmp, 0, nread);
        mrbuff.flip();
        delete[](tmp);
    }

    ssize_t count = 0;
    while(mrbuff.has_remaining() && dst.has_remaining()){
        dst.put(mrbuff.get());
        count++;
    }
    return count;
}

ssize_t SocketChannel::write(ByteBuffer &src) {
    ssize_t count = 0;
    //Todo: should refine write to buffering in mwbuff, instead of sending all bytes to network
    while(mwbuff.has_remaining() || src.has_remaining()){
        // Emptying mwbuff
        while(mwbuff.has_remaining()){
            //Todo: replace string with byte array smart pointer
            std::string tmp = mwbuff.to_string();
            ssize_t nwrite = ::write(msocket.get_socketfd(),
                    (void*)tmp.c_str(), tmp.size());
            if(nwrite < 0){
                return -1;
            }
            count += nwrite;
        }
        // Preparing reading from src to mwbuff
        mwbuff.clear();

        while(mwbuff.has_remaining() && src.has_remaining()){
            mwbuff.put(src.get());
        }
        mwbuff.flip();
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











