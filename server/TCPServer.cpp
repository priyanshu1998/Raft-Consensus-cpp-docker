#include <cctype>
#include <cerrno>
#include <cstring>
#include <cstdio>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include "TCPServer.h"

//int queueSize = 100;

char* TCPServer::inetAddressStr(const struct sockaddr *addr, socklen_t addrlen,
                                char *addrStr, int addrStrLen){
    char host[NI_MAXHOST], service[NI_MAXSERV];

    if(getnameinfo(addr, addrlen, host, NI_MAXSERV,
                   service, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0){
        snprintf(addrStr, addrStrLen, "(%s, %s)", host, service);
    }else{
        snprintf(addrStr, addrStrLen, "(?UNKNOWN?)");
    }
    addrStr[addrStrLen-1] = 0;
    return addrStr;
}

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
               inetAddressStr(bind_address->ai_addr, bind_address->ai_addrlen, addrStr, 1024));

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

    FD_SET(this->sockfd, &(this->masterfds));

    _serveForever();
}

void TCPServer::_serveForever() {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    for(;;){
        fd_set readfds = this->masterfds;
        if(select(FD_SETSIZE, &readfds, nullptr, nullptr, nullptr) < 0){
            int errsv = errno;
            fprintf(stderr,"[E|-Select Syscall-%s-%d] %s\n", __func__, errsv, strerror(errsv));
        }

        for(int i=0; i<FD_SETSIZE; i++){
            if(FD_ISSET(i, &readfds)){
                if(i == this->sockfd){
                    // sockfd has data.
                    struct sockaddr_in clientAddress{};
                    int commSock = _acceptClientWrapper(clientAddress);
                    if(commSock == -1){
                        int errsv = errno;
                        fprintf(stderr,"[E|-Client Not Connected-%s-%d] %s\n", __func__, errsv, strerror(errsv));
                    }
                    postConnectRoutine(commSock, clientAddress);
                }else{
                    // one of the socket associated with client has data.
                    int stat = _handleRequest(i);
                    if(stat != 0){
                        preCloseRoutine(i);
                        close(i);
                    }
                }
            }
        }
    }
#pragma clang diagnostic pop
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
           inetAddressStr((struct sockaddr*)&clientAddress, sizeof(clientAddress), addrStr, 1024), clientSock);

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


    if(send(clientSock, send_buf, 1023, 0) == -1){
        int errsv = errno;
        fprintf(stderr,"[E-Send error from-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        return errsv;
    }
    return 0;
}

void TCPServer::postConnectRoutine(int commSock, const struct sockaddr_in &clientAddr) {
    std::string msg = "connected to "+std::string(this->name);
    send(commSock, msg.c_str(), msg.length(), 0);
    
    FD_SET(commSock, &(this->masterfds));
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
    }else{
        char terminatingClient[1024]{};
        fprintf(stderr, "[I| Client %s teminated.\n", inetAddressStr( (struct sockaddr*)&addrinfo, addrsize, terminatingClient, 1024));
    }

    FD_CLR(commSock, &(this->masterfds));
}

