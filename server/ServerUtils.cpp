//
// Created by hiro on 3/17/22.
//

#include "ServerUtils.h"

bool ServerUtils::readIP(int fd, char *IP) {
    char c = 0;
    int idx = 0;
    while (c != '\n') {
        if (read(fd, &c, 1) == -1) {
            int errsv = errno;
            fprintf(stderr, "[E| unable to read IP] %s\n", strerror(errsv));
            return false;
        }
        if(c=='\n'){continue;}
        IP[idx] = c;
        idx++;
    }
    return true;
}
