
#include "RaftServer.h"
#include "ServerUtils.h"
#include <unistd.h>
#include <cstdio>

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

    char IP[NI_MAXHOST]{};
    if(getnameinfo((struct sockaddr*)&clientAddress, sizeof (clientAddress), IP, NI_MAXHOST,
                nullptr, 0, NI_NUMERICHOST) != 0){
        int errsv = errno;
        fprintf(stderr, "[E|didnt get hostname in post connect stage] %s\n", strerror(errsv));
    }

    //Add to monitoredPeer if it's a server.
    if(ServerUtils::IP2Hostname.find(IP) != ServerUtils::IP2Hostname.end()){
        this->monitoredPeer[commSock] = IP;
    }
}

void RaftServer::preCloseRoutine(int commSock){
    printf("RAFT SERVER Routine called\n");
    if(RaftServer::isServerRequest(commSock)){
        std::string IP = this->monitoredPeer[commSock];

        this->monitoredPeer.erase(commSock);
        this->connectedPeer.erase(IP);
        fprintf(stderr, "IP TO CONNECT %s| %s\n", IP.c_str(), ServerUtils::IP2Hostname[IP].c_str());
        TCPServer::preCloseRoutine(commSock);
        lazyConnect(ServerUtils::IP2Hostname[IP].c_str());
        return;
    }
    TCPServer::preCloseRoutine(commSock);
}

bool RaftServer::descriptorEvents(fd_set &readfds, int i) {
    return TCPServer::descriptorEvents(readfds, i);
}

bool RaftServer::tryConnecting(const char *host) {
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
    this->toConnect.emplace_back(host);
}

bool RaftServer::isServerRequest(int fd) {
    return this->monitoredPeer.find(fd) != this->monitoredPeer.end();
}

int RaftServer::_handleRequest(int clientSock) {
    if(RaftServer::isServerRequest(clientSock)){
        fprintf(stderr, "[I] request from server\n");
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
        fprintf(stderr,"GOT from server %s\n", recv_buf);
        return 0;
    }

    return TCPServer::_handleRequest(clientSock);
}

void RaftServer::timeoutEvent() {
    //try to connect to other servers if any========
    if(!this->toConnect.empty()){
        size_t size = toConnect.size();

        for(int i=0; i<size; i++){
            auto hostname = toConnect.front();
            toConnect.pop_front();
            bool isConnected = tryConnecting(hostname.c_str());

            if(!isConnected){
                toConnect.push_back(hostname);
            }
        }
    }
    //end of routine=================================


    char msg[] = "Hello!!!\n";
    TCPServer::timeoutEvent();
    ServerUtils::broadcast(this->connectedPeer, msg, strlen(msg));
}
