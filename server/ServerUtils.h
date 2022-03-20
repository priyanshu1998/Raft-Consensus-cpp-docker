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
#include <sys/socket.h>

namespace ServerUtils{
    constexpr char PORT[] = "3000";
    constexpr const int TOT_SERVERS = 3;

    bool readIP(int fd ,char *IP);

    static std::map<std::string , std::string > IP2Hostname;
    bool broadcast(std::map<std::string, int> &connectedPeer, void *payload, int payloadSize);

    int nextEventDescriptor(std::deque<int>&eventQueue);

    int addEventDescriptor(std::deque<int>&eventQueue, int fd, fd_set &readfds);

    char* inetAddressStr(sockaddr* addr, socklen_t addrlen,
                                      char *addrStr, int addrStrLen);
    bool addServerToListOfKnowHost(const char* hostname);

    bool isPeerPresent(int fd);

}
#endif //RAFT_CPP_SERVERUTILS_H
