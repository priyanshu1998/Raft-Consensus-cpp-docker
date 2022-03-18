
#include "RaftServer.h"
#include "ServerUtils.h"
#include <unistd.h>

// https://isocpp.org/wiki/faq/strange-inheritance#calling-virtuals-from-base

static constexpr char PORT[] = "3000";
static char delimiter = '\n';

#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)


static void append_delimiter(char *s){
    if(s[strlen(s)-1] != delimiter) {
        size_t len = strlen(s);
        s[len] = delimiter;
        s[len + 1] = 0;
    }
}

void RaftServer::postConnectRoutine(int commSock, const sockaddr_in &clientAddress) {
    TCPServer::postConnectRoutine(commSock, clientAddress);
//    char hostname[NI_MAXHOST]{};
//
//    if(getnameinfo((struct sockaddr*)&clientAddress, NI_MAXHOST, hostname, NI_MAXHOST,
//                nullptr, 0, 0) == 0){
//        append_delimiter(hostname);
//        write(this->tryConnectPipe[1], hostname, strlen(hostname));
//    }
}

void RaftServer::preCloseRoutine(int commSock){
    printf("RAFT SERVER Routine called\n");
    TCPServer::preCloseRoutine(commSock);
}

bool RaftServer::descriptorEvents(fd_set &readfds) {
    // New host/IP to connect
    if (FD_ISSET(this->tryConnectPipe[0], &readfds)) {
        char IP[1024]{};
        ServerUtils::readIP(this->tryConnectPipe[0], IP);
        bool isConnectionSuccessful = tryConnecting(IP);

        if(!isConnectionSuccessful){
            append_delimiter(IP);
            write(this->tryConnectPipe[1], IP, strlen(IP));
        }

        eventQueue.push(this->tryConnectPipe[0]);
        return true;
    }
    return TCPServer::descriptorEvents(readfds);
}

bool RaftServer::tryConnecting(char *host) {
    struct addrinfo hints{
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *result = nullptr;
    int stat = getaddrinfo(host, PORT, &hints, &result);
    if(stat != 0){
        fprintf(stderr,"[E|-getaddrinfo %d] %s\n", s, gai_strerror(s));
//        _exit(EXIT_FAILURE);
        return false;
    }

    char address[ADDRSTRLEN]{};
    printf("[I| Will try to connect to %s]\n", inetAddressStr(result->ai_addr, result->ai_addrlen,
                                        address, ADDRSTRLEN));

    int sockfd = socket(result->ai_family, result->ai_socktype,
                        result->ai_protocol);

    if(sockfd == -1){
        int errsv = errno;
        fprintf(stderr,"[E|-creating socket-%d] %s\n", errsv, strerror(errsv));
        return false;
    }


    if(connect(sockfd, result->ai_addr, result->ai_addrlen) == -1){
        int errsv = errno;
        fprintf(stderr, "[E|-connecting to server- %d] %s\n", errsv, strerror(errsv));
        return false;
    }

    printf("[I| Connection successful]\n");
    return true;
}

void RaftServer::lazyConnect(char *host) {
    write(this->tryConnectPipe[1], host, strlen(host));
}
