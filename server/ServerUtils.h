//
// Created by hiro on 3/17/22.
//

#ifndef RAFT_CPP_SERVERUTILS_H
#define RAFT_CPP_SERVERUTILS_H

#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <deque>

namespace ServerUtils{
    constexpr int TOT_SERVERS = 3;

    bool readIP(int fd ,char *IP);

    struct Conn{
        int acceptSock{-1};
        int connectSock{-1};
    };

    bool insertConnectSock(std::map<std::string,Conn> &peerEndpoints, const char *host, int sockfd);

    bool insertAcceptSock(std::map<std::string,Conn> &peerEndpoints, const char *host, int sockfd);

    int nextEventDescriptor(std::deque<int>&eventQueue);

    int addEventDescriptor(std::deque<int>&eventQueue, int fd, fd_set &readfds);
}
#endif //RAFT_CPP_SERVERUTILS_H
