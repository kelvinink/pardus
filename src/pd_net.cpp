#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>

#include "pd_net.h"

/*
 * pd_connect - Open connection to server at <hostname, port> and
 *     return a socket descriptor ready for reading and writing. This
 *     function is reentrant and protocol-independent.
 *     On error, returns -1 and sets errno.  
 */
int pd_connect(const std::string& hostname, const std::string& port){
    int connectfd;
    addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV;  /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    getaddrinfo(hostname.c_str(), port.c_str(), &hints, &listp);
  
    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p != nullptr; p = p->ai_next) {
        if ((connectfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;
        if (connect(connectfd, p->ai_addr, p->ai_addrlen) != -1) 
            break; /* Success */
        close(connectfd); 
    } 

    freeaddrinfo(listp);
    if (!p) 
        return -1;
    else
        return connectfd;
}

/*  
 * pd_listen - Open and return a listening socket on port. 
 * This function is reentrant and protocol-independent.
 *
 *     On error, returns -1 and sets errno.
 */
int pd_listen(const std::string& port){
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Accept TCP connections */
    hints.ai_flags = AI_PASSIVE;      /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV; /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    getaddrinfo(NULL, port.c_str(), &hints, &listp);

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */
        close(listenfd); 
    }

    freeaddrinfo(listp);
    if (!p)
        return -1;

    if (listen(listenfd, LISTENQ) < 0)
	return -1;
    return listenfd;
}