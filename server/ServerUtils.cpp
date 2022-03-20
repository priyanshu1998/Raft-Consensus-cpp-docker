//
// Created by hiro on 3/17/22.
//

#include "ServerUtils.h"

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

bool ServerUtils::insertAcceptSock(std::map<std::string, Conn> &peerEndpoints, const char *host, int sockfd){
    auto it = peerEndpoints.find(host);
    if(it != peerEndpoints.end()){
        it->second.acceptSock = sockfd;

        // the fact that it was found implies .connectSock exist
        // therefore both the listening sockets have established connection.

        return true;
    }else{
        peerEndpoints[host] = ServerUtils::Conn{};
        peerEndpoints[host].acceptSock = sockfd;
        return false;
    }
}

bool ServerUtils::insertConnectSock(std::map<std::string, Conn> &peerEndpoints, const char *host, int sockfd) {
    auto it = peerEndpoints.find(host);
    if(it != peerEndpoints.end()){
        it->second.connectSock = sockfd;

        // the fact that it was found implies .acceptSock exist
        // therefore both the listening sockets have established connection.
        return true;
    }else{
        peerEndpoints[host] = ServerUtils::Conn{};
        peerEndpoints[host].connectSock = sockfd;
        return false;
    }
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
}