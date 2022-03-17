//
// Created by hiro on 3/17/22.
//

#ifndef RAFT_CPP_SERVERUTILS_H
#define RAFT_CPP_SERVERUTILS_H

#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace ServerUtils{
    bool readIP(int fd ,char *IP);


}
#endif //RAFT_CPP_SERVERUTILS_H
