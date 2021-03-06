#ifndef ASSIGNMENT3_TCPSERVER_H
#define ASSIGNMENT3_TCPSERVER_H

#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <queue>
#include <sys/select.h>
#include <cstdio>

class TCPServer{
protected:
    int sockfd = -1;
    fd_set masterfds{};
    std::deque<int>eventQueue;


    virtual void postConnectRoutine(int commSock,const struct sockaddr_in &clientAddress);
    virtual void preCloseRoutine(int commSock);
    virtual int _handleRequest(int clientSock);
    virtual void timeoutEvent(){fprintf(stderr,"[I] Timeout occurred.\n");};

private:
    void _createAndBind(const char *hostname, const char *port) ;
    void _listenAndAccept();
    int _acceptClientWrapper(struct sockaddr_in &clientAddress) const;

public:
    TCPServer(const char *hostname, const char *port) {
        _createAndBind(hostname, port);
        FD_ZERO(&(this->masterfds));
    }

    explicit TCPServer(const char* port){
        _createAndBind(nullptr, port);
        FD_ZERO(&(this->masterfds));

    }

    void actAsServer();


    void _serveForever();

    virtual bool descriptorEvents(fd_set &readfds, int i);
};
#endif //ASSIGNMENT3_TCPSERVER_H
