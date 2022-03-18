#ifndef ASSIGNMENT3_TCPSERVER_H
#define ASSIGNMENT3_TCPSERVER_H

#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <queue>
#include <sys/select.h>

class TCPServer{
protected:
    int sockfd = -1;
    fd_set masterfds{};
    std::queue<int>eventQueue;


    virtual void postConnectRoutine(int commSock,const struct sockaddr_in &clientAddress);
    virtual void preCloseRoutine(int commSock);
private:
    void _createAndBind(const char *hostname, const char *port) ;
    void _listenAndAccept();
    static int _handleRequest(int clientSock) ;
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

    static char *inetAddressStr(const struct sockaddr *addr, socklen_t addrlen,
                         char *addrStr, int addrStrLen);


    void _serveForever();

    virtual bool descriptorEvents(fd_set &readfds, int i);
};
#endif //ASSIGNMENT3_TCPSERVER_H
