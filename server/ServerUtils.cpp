//
// Created by hiro on 3/17/22.
//

#include "ServerUtils.h"
#include <netdb.h>
#include <sys/socket.h>


char* ServerUtils::inetAddressStr(struct sockaddr *addr, socklen_t addrlen,
                                char *addrStr, int addrStrLen){
    char host[NI_MAXHOST], service[NI_MAXSERV];

    if(getnameinfo(addr, addrlen, host, NI_MAXHOST,
                   service, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV) == 0){
//                   service, NI_MAXSERV, 0) == 0){
        snprintf(addrStr, addrStrLen, "(%s, %s)", host, service);
    }else{
        int errsv = errno;
        fprintf(stderr, "[E| could not resolve hostname] %s\n", strerror(errsv));
        snprintf(addrStr, addrStrLen, "(?UNKNOWN?)");
    }
    addrStr[addrStrLen-1] = 0;
    return addrStr;
}

bool ServerUtils::readIP(int fd, char *IP) {
    char c = 0;
    int idx = 0;
    for(;;){
        if (read(fd, &c, 1) == -1) {
            int errsv = errno;
            fprintf(stderr, "[E| unable to read IP] %s\n", strerror(errsv));
            return false;
        }
        if(c=='\n'){ // '\n' is delimiter
            IP[idx] = 0;
            break;
        }
        IP[idx] = c;
        idx++;
    }
    return true;
}


int ServerUtils::nextEventDescriptor(std::deque<int> &eventQueue) {
    int i = eventQueue.front();
    eventQueue.pop_front();
    eventQueue.push_back(i);

    return i;
}

int ServerUtils::addEventDescriptor(std::deque<int> &eventQueue, int fd, fd_set &readfds) {
    eventQueue.push_back(fd);
    FD_SET(fd, &readfds);
    return fd;
}

bool ServerUtils::addServerToListOfKnowHost(const char *hostname) {
    struct addrinfo hints{
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *result = nullptr;
    int stat = getaddrinfo(hostname, ServerUtils::PORT, &hints, &result);
    if(stat != 0){
        fprintf(stderr,"[E|-getaddrinfo %d] %s\n", stat, gai_strerror(stat));
//        _exit(EXIT_FAILURE);
        return false;
    }

    while(result != nullptr && result->ai_family != AF_INET){
        result = result->ai_next;
    }


    char hostIP[NI_MAXHOST];
    if(getnameinfo(result->ai_addr, result->ai_addrlen, hostIP, NI_MAXHOST,
                   nullptr, NI_MAXSERV, NI_NUMERICHOST) == 0){
//                   nullptr, 0, NI_NOFQDN) == 0){
        fprintf(stderr,"[I] in list %s\n", hostIP);
    }else{
        fprintf(stderr, "ERROR\n");
    }

    ServerUtils::IP2Hostname[hostIP] = hostname;
    return true;
}

bool ServerUtils::isPeerPresent(int peer_fd) {
    sockaddr_in addrInfo{};
    socklen_t len{};

    if(getpeername(peer_fd, (struct sockaddr*) &addrInfo, &len) == -1){
        int errsv = errno;
        if(errsv == 107){
            fprintf(stderr, "[I | client terminated before receiving response]\n");
            return false;
        }

        fprintf(stderr, "[E | isPeerPresent some other error occurred] <%d> %s\n", errsv, strerror(errsv));
        return false;
    }
    return true;
}

bool ServerUtils::broadcast(std::map<std::string, int> &connectedPeer, void *payload, size_t payloadSize) {
    bool delivered_to_all = true;

    for(const auto& IP_fd_pair: connectedPeer){
        std::string IP{};
        int fd{};

        std::tie(IP, fd) = IP_fd_pair;
        if(isPeerPresent(fd)){
            send(fd, payload, payloadSize, 0);
        }

        //TODO: in progress check for situations where the client server disconnects.
    }

    return delivered_to_all;
}

