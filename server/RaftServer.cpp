
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
    char hostname[NI_MAXHOST]{};

    if(getnameinfo((struct sockaddr*)&clientAddress, sizeof (clientAddress), hostname, NI_MAXHOST,
                nullptr, 0, NI_NAMEREQD) == 0){
//        bool isConnectedBothWays = ServerUtils::insertAcceptSock(this->peerEndpoints, hostname, commSock);
//        if(isConnectedBothWays && this->s == Initializing){
//            fprintf(stderr, "[I] Connected Both Ways.\n");
//            this->totalEstablishedConn++;
//            if(totalEstablishedConn > ServerUtils::TOT_SERVERS/2){
//                this->s = Follower;
//            }
//        }else{
//            fprintf(stderr, "[I] connected as client only.\n");
//        }
    }else{
        int errsv = errno;
        fprintf(stderr, "[E|didnt get hostname in post connect stage] %s\n", strerror(errsv));
    }
}

void RaftServer::preCloseRoutine(int commSock){
    printf("RAFT SERVER Routine called\n");
    TCPServer::preCloseRoutine(commSock);
}

bool RaftServer::descriptorEvents(fd_set &readfds, int i) {
    // New host/IP to connect
    if (i == this->tryConnectPipe[0]) {
        char IP[1024]{};
        ServerUtils::readIP(this->tryConnectPipe[0], IP);
        bool isConnectionSuccessful = tryConnecting(IP);

        if(!isConnectionSuccessful){
            append_delimiter(IP);
            write(this->tryConnectPipe[1], IP, strlen(IP));
        }

        return true;
    }
    return TCPServer::descriptorEvents(readfds, i);
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

    while(result != nullptr && result->ai_family != AF_INET){
        result = result->ai_next;
    }

    char address[ADDRSTRLEN]{};
    fprintf(stderr, "[I| Will try to connect to %s]\n", ServerUtils::inetAddressStr(result->ai_addr, result->ai_addrlen,
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

//    bool isConnectedBothWays = ServerUtils::insertConnectSock(peerEndpoints, host, sockfd);
//    if(isConnectedBothWays && this->s == Initializing){
//        fprintf(stderr,"[I] Connected Both Ways.\n");
//        this->totalEstablishedConn++;
//        if(totalEstablishedConn > ServerUtils::TOT_SERVERS/2){
//            this->s = Follower;
//        }
//    }

    char IP[NI_MAXHOST]{};
    if(getnameinfo(result->ai_addr, result->ai_addrlen, IP, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST)  == 0){
        fprintf(stderr, "[I| Connection successful] connected to %s\n", IP);
        this->connectedPeer[IP] = sockfd;   //note: one to many mapping considering all process
        ServerUtils::IP2Hostname[IP] = host; // note: one to one mapping across all processes.
        return true;
    }
    fprintf(stderr, "[W| Connection successful but IP to fd/host translation not saved]\n");
    return false;
}

void RaftServer::lazyConnect(const char *host) {
    char t_host[1024]{};
    strcpy(t_host, host);
    append_delimiter(t_host);

    write(this->tryConnectPipe[1], t_host, strlen(t_host));
}

bool RaftServer::isServerRequest(int fd) {
    // If bound to Port "3000" ServerUtils::PORT then the request is from a server.
    struct sockaddr_in addr{};
    socklen_t socklen = sizeof (struct sockaddr_in);
    if(getsockname(fd, (struct sockaddr*)&addr, &socklen) == -1){
        int errsv = errno;
        fprintf(stderr, "[E | failed to the sockaddr_in ] %s\n", strerror(errsv));
        return false;
    }

    char service[NI_MAXSERV]{};
    if(getnameinfo((struct sockaddr*)&addr, socklen, nullptr, 0, service, NI_MAXSERV, NI_NUMERICSERV) == -1){
        int errsv = errno;
        fprintf(stderr, "[E | unable to obtain the service port] %s\n", strerror(errsv));
        return false;
    }

    return strcmp(service, ServerUtils::PORT) == 0;
}
