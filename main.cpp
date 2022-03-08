#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <cctype>

#include <unistd.h>
#include <netdb.h>


#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT "5000"

#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)

static char *inetAddressStr(const struct sockaddr *addr, socklen_t addrlen,
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

static void obtainDataFromPipe(char *buf, int pipefd0){
    int idx = 0;
    char cbuf = 0;
    while(read(pipefd0, &cbuf, 1) > 0){
        if(cbuf == '\n'){
            buf[idx] = 0;
            break;
        }
        buf[idx++] = cbuf;
    }
}


//static void grimReaper(int sig){
//    int savedErrno = errno;
//    while(waitpid(-1, nullptr, WNOHANG) > 0);
//    errno = savedErrno;
//}

bool prefixMatch(const char* cmdStr,const char* prefix){
    size_t n = strlen(prefix);
    return strncmp(cmdStr, prefix, n) == 0;
}

static void parseCmd(int sockfd, const char* cmdStr){

    printf("Unknown command received.\n");
}

void childCodeBlock(int pipefd[2]){
    struct addrinfo hints{
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *result = nullptr;
    int s = getaddrinfo(SERVER_ADDRESS, SERVER_PORT, &hints, &result);
    if(s != 0){
        printf("[E|-getaddrinfo %d] %s\n", s, gai_strerror(s));
        _exit(EXIT_FAILURE);
    }

    char address[ADDRSTRLEN]{};
    printf("[I| Will try to connect to %s]\n", inetAddressStr(result->ai_addr, result->ai_addrlen,
                                        address, ADDRSTRLEN));

    int sockfd = socket(result->ai_family, result->ai_socktype,
                        result->ai_protocol);

    if(sockfd == -1){
        int errsv = errno;
        printf("[E|-creating socket-%d] %s\n", errsv, strerror(errsv));
    }


    if(connect(sockfd, result->ai_addr, result->ai_addrlen) == -1){
        int errsv = errno;
        printf("[E|-connecting to server- %d] %s\n", errsv, strerror(errsv));
        _exit(EXIT_FAILURE);
    }

    printf("[I| Connection successful]\n");

    fd_set master;
    FD_ZERO(&(master));

    FD_SET(pipefd[0], &master);
    FD_SET(sockfd, &master);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    for(;;){
        fd_set readfds = master;

        if(select(FD_SETSIZE, &readfds, nullptr, nullptr, nullptr) < 0){
            int errsv = errno;
            printf("[E|-calling select-%d] %s\n", errsv, strerror(errsv));
        }
//        printf("IN LOOP\n");

        for(int i=0; i<FD_SETSIZE; i++){
            if(FD_ISSET(i, &readfds)){
                if(i == sockfd){
                    // response/invitation from the server
                    char recvBuf[1024] = {0};
                    size_t n = recv(sockfd, recvBuf, 1023, 0);
                    printf("RECEIVED FOR SOCKET: %s\n", recvBuf);
                    if(n < 0){
                        int errsv = errno;
                        printf("[E|-issue at recv-%d] %s\n", errsv, strerror(errsv));
                    }
                }else{
                    // cmd request/invitation response from client.
                    char reqBuf[1024]{};
                    obtainDataFromPipe(reqBuf, pipefd[0]);  //reqBuf will not have newline char
                    printf("RECEIVED FROM PIPE: %s\n",  reqBuf);
                    if(strncmp(reqBuf, "EXIT", 4) == 0){
                        return;
                    }

                    //forward the request/invitation response to the server
//                    parseCmd(sockfd,reqBuf);
                    send(sockfd, reqBuf, strlen(reqBuf), 0);

//                    char sendBuf[1024]{};
//                    for(int idx=0; idx< strlen(reqBuf); idx++){
//                        sendBuf[idx] = toupper(reqBuf[idx]);
//                    }
//                    send(sockfd, sendBuf, strlen(sendBuf), 0);

                }
            }
        }
    }
#pragma clang diagnostic pop
}

void parentCodeBlock(int pipefd[2]){
    close(pipefd[0]);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    for(;;){
        char *line = nullptr;
        size_t lineSize = 0;

        getline(&line, &lineSize, stdin);
        write(pipefd[1], line, strlen(line));
        if(strncmp(line, "EXIT", 4) == 0){
            return;
        }
    }
#pragma clang diagnostic pop
}

int main() {
    int pipefd[2]{};
    if(pipe(pipefd)==-1){
        int errsv = errno;
        printf("[E|-pipe error-%d] %s\n", errsv, std::strerror(errsv));
        return -1;
    }

    pid_t cpid = fork();
    if(cpid == -1){
        int errsv = errno;
        printf("[E|-fork error-%d] %s\n", errsv, std::strerror(errsv));
        return -1;
    }

    if(cpid == 0){
        close(pipefd[1]);   // close write end for the child.
        childCodeBlock(pipefd);
        close(pipefd[0]);
        printf("Child Process is Exiting\n");
        _exit(EXIT_SUCCESS);
    }else{
        close(pipefd[0]);   //close read end for the parent
        parentCodeBlock(pipefd);
        close(pipefd[1]);
        wait(nullptr);
        printf("Parent Process is Exiting\n");
    }
    exit(EXIT_SUCCESS);
}
