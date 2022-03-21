#include <cctype>
#include <cerrno>
#include <cstring>
#include <cstdio>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "TCPServer.h"
#include "ServerUtils.h"

//int queueSize = 100;


void TCPServer::_createAndBind(const char *hostname, const char* port) {
    struct addrinfo hints{
            .ai_flags = AI_PASSIVE, /* For wildcard IP address */
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *bind_address = nullptr;
    {
        int s = getaddrinfo(hostname, port, &hints, &bind_address);
        if (s != 0) {
            fprintf(stderr,"[E|-Socket Creation-%s-%d] %s\n", __func__, s, gai_strerror(s));
        }
        // /*******************************************
        //Only use IPv4 address type

        while (bind_address != nullptr && bind_address->ai_family != AF_INET){
            bind_address = bind_address->ai_next;
        }
        // ********************************************/

//        fprintf(stderr,"Creating socket...\n");
        this->sockfd = socket(bind_address->ai_family,
                              bind_address->ai_socktype, bind_address->ai_protocol);
        if (this->sockfd == -1) {
            int errsv = errno;
            fprintf(stderr,"[E|-Socket Creation-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        }

        if (bind(this->sockfd, bind_address->ai_addr, bind_address->ai_addrlen) == -1) {
            int errsv = errno;
            fprintf(stderr,"[E|-Socket Binding-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
            close(sockfd);
        }

        char addrStr[1024]{};
        fprintf(stderr,"[I|-Socket Created and Binded] %s\n",
               ServerUtils::inetAddressStr(bind_address->ai_addr, bind_address->ai_addrlen, addrStr, 1024));

    }
    freeaddrinfo(bind_address);
}

void TCPServer::actAsServer() {
    _listenAndAccept();
}

void TCPServer::_listenAndAccept() {
    if(listen(this->sockfd, 100)){
        int errsv = errno;
        fprintf(stderr,"[E|-Socket Listen-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        close(this->sockfd);
    }
    fprintf(stderr,"[I|-Socket in LISTENING state-] can accept new connections.\n"),


    ServerUtils::addEventDescriptor(this->eventQueue, this->sockfd, this->masterfds);

    _serveForever();
}

void TCPServer::_serveForever() {
    struct timeval timeout_interval{.tv_usec = 5000000};
    auto &interval = timeout_interval.tv_usec;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    for(;;){
        fd_set readfds = this->masterfds;
        if(interval == 0){interval = 5000000;}

        if(select(FD_SETSIZE, &readfds, nullptr, nullptr, &timeout_interval) < 0){
            int errsv = errno;
            fprintf(stderr,"[E|-Select Syscall-%s-%d] %s\n", __func__, errsv, strerror(errsv));
        }

        if(interval == 0){
            fprintf(stderr,"Timeout occurred\n");

            timeoutEvent();
            continue;
        }

        int i = ServerUtils::nextEventDescriptor(eventQueue);
        if(FD_ISSET(i, &readfds)){
            bool isHandled = descriptorEvents(readfds, i);

            if(!isHandled) {
                fprintf(stderr, "[W] Invoking default handler\n");

                // one of the socket associated with client has data.
                int stat = _handleRequest(i);
                if (stat != 0) {
                    preCloseRoutine(i);
                    close(i);
                }
            }
        }
    }
#pragma clang diagnostic pop
}

bool TCPServer::descriptorEvents(fd_set &readfds, int i) {
    if(i == this->sockfd){
        // New connection request
        fprintf(stderr,"[I] New client connected.\n");
        struct sockaddr_in clientAddress{};
        int commSock = _acceptClientWrapper(clientAddress);

        if(commSock == -1){
            int errsv = errno;
            fprintf(stderr,"[E|-Client Not Connected-%s-%d] %s\n", __func__, errsv, strerror(errsv));
        }
        postConnectRoutine(commSock, clientAddress);

        return true;
    }
    return false;
}

int TCPServer::_acceptClientWrapper(struct sockaddr_in &clientAddress) const {
    socklen_t addrLen = sizeof(clientAddress);
    int clientSock = accept(this->sockfd, (struct sockaddr*)&clientAddress, &addrLen);

    if(clientSock == -1){
        int errsv = errno;
        fprintf(stderr,"[E|-accept conn-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        return -1;
    }

    char addrStr[1024]{};
    fprintf(stderr,"[I]-accepted connection from %s on fd: %d\n",
           ServerUtils::inetAddressStr((struct sockaddr*)&clientAddress, sizeof(clientAddress), addrStr, 1024), clientSock);

    return clientSock;
}

int TCPServer::_handleRequest(int clientSock) {
    char recv_buf[1024]{};

    if(recv(clientSock, recv_buf, 1023, 0) <= 0){
        int errsv = errno;
        if(errsv == 104){
            // Transport endpoint is not connected
            return -1;
        }else if(errsv == 0){
            // Success with zero length payload.
            return -1;
        }

        fprintf(stderr,"[E-Recv error from-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        return errsv;
    }

    if(recv_buf[strlen(recv_buf)-1] == '\n'){
        // remove newline from the request.
        recv_buf[strlen(recv_buf)-1] = 0;
    }

    char send_buf[1024]{};

    for(int i=0; i< std::strlen(recv_buf); i++){
        send_buf[i] = toupper(recv_buf[i]);
    }
    fprintf(stderr, "[INFO] %s\n", send_buf);


    if(!ServerUtils::isPeerPresent(clientSock) || send(clientSock, send_buf, 1023, 0) == -1){
        int errsv = errno;
        fprintf(stderr,"[E-Send error from-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        return errsv;
    }
    return 0;
}

void TCPServer::postConnectRoutine(int commSock, const struct sockaddr_in &clientAddr) {
    ServerUtils::addEventDescriptor(this->eventQueue, commSock, this->masterfds);
}

void TCPServer::preCloseRoutine(int commSock) {
    //TODO: Implement doubly connected linklist to keep track of the (sockfd, sockaddr) pairs.
    // for now using fd to keep track of peers.
    sockaddr_in addrinfo{};
    socklen_t addrsize =  sizeof(sockaddr_in);

    if(getpeername(commSock, (struct sockaddr *)&addrinfo, &addrsize) == -1){
        int errsv = errno;

        if(errsv == 107){
            // Transport endpoint is not connected
            fprintf(stderr, "[I] Client terminated on fd: %d\n", commSock);
        }else {
            fprintf(stderr, "[E| unable to get sockname] %s\n", strerror(errsv));
        }
        goto release_fd;
    }else{
        char terminatingClient[1024]{};
        fprintf(stderr, "[I| Client %s terminated.\n", ServerUtils::inetAddressStr( (struct sockaddr*)&addrinfo, addrsize, terminatingClient, 1024));
    }

    release_fd:
    printf("Socket removed from masterfds.\n");
    /** popEventDescriptor *****************/
    FD_CLR(commSock, &(this->masterfds));
    this->eventQueue.pop_back();
    /**************************************/
}

