#include "TCPServer.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>

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
            printf("[E|-Socket Creation-%s-%d] %s\n", __func__, s, gai_strerror(s));
        }
//        printf("Creating socket...\n");
        this->sockfd = socket(bind_address->ai_family,
                              bind_address->ai_socktype, bind_address->ai_protocol);
        if (this->sockfd == -1) {
            int errsv = errno;
            printf("[E|-Socket Creation-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        }

        if (bind(this->sockfd, bind_address->ai_addr, bind_address->ai_addrlen) == -1) {
            int errsv = errno;
            printf("[E|-Socket Binding-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
            close(sockfd);
        }

        char addrStr[1024]{};
        printf("[I|-Socket Created and Binded] %s\n",
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
        printf("[E|-Socket Listen-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        close(this->sockfd);
    }
    printf("[I|-Socket in LISTENING state-] can accept new connections.\n"),

            FD_SET(this->sockfd, &(this->currentSockets));

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    for(;;){
        fd_set  readfds = this->currentSockets;
        if(select(FD_SETSIZE, &readfds, nullptr, nullptr, nullptr) < 0){
            int errsv = errno;
            printf("[E|-Select Syscall-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        }

        for(int i=0; i<FD_SETSIZE; i++){
            if(FD_ISSET(i, &readfds)){
                if(i == this->sockfd){
                    // sockfd has data.
                    struct sockaddr_in clientAddress{};
                    int commSock = _acceptClientWrapper(clientAddress);
                    if(commSock == -1){
                        int errsv = errno;
                        printf("[E|-Client Not Connected-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
                    }
                    postConnectRoutine(commSock, clientAddress);
                }else{
                    // one of the socket associated with client has data.
                    bool stat = handleRequest(i);
                    if(!stat){
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
        printf("[E|-accept conn-%s-%d] %s\n", __func__, errsv, std::strerror(errsv));
        return -1;
    }

    char addrStr[1024]{};
    printf("[I]-accepted connection from %s\n",
           inetAddressStr((struct sockaddr*)&clientAddress, sizeof(clientAddress), addrStr, 1024));

    return clientSock;
}

void TCPServer::postConnectRoutine(int commSock,const struct sockaddr_in &clientAddr) {}

void TCPServer::preCloseRoutine(int commSock) {
    FD_CLR(commSock, &(this->currentSockets));
}

bool TCPServer::handleRequest(int peer_sockfd) {
    return false;
}
