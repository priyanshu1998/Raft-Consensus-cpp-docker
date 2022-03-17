#ifndef ASSIGNMENT3_TCPSERVER_H
#define ASSIGNMENT3_TCPSERVER_H

#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/select.h>

class TCPServer{
protected:
    char name[128]{};
    int sockfd = -1;
private:
    fd_set masterfds{};

    void _createAndBind(const char *hostname, const char *port) ;
    void _listenAndAccept();
    static int _handleRequest(int clientSock) ;
    int _acceptClientWrapper(struct sockaddr_in &clientAddress) const;
    void postConnectRoutine(int commSock,const struct sockaddr_in &clientAddress);
    void preCloseRoutine(int commSock);

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
};
#endif //ASSIGNMENT3_TCPSERVER_H
