#ifndef RAFT_CPP_TCPSERVER_H
#define RAFT_CPP_TCPSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/select.h>

 class TCPServer{
private:
    int sockfd = -1;
    fd_set currentSockets{};
    void _createAndBind(const char *hostname, const char *port) ;
    static char* inetAddressStr(const struct sockaddr *addr, socklen_t addrlen,
                                    char *addrStr, int addrStrLen);

    void _listenAndAccept();



public:
    TCPServer(const char *hostname, const char *port) {
        _createAndBind(hostname, port);
        FD_ZERO(&(this->currentSockets));
    }

    virtual void actAsServer();
    virtual bool handleRequest(int peer_sockfd);
    virtual void preCloseRoutine(int peer_sockfd);
    virtual void postConnectRoutine(int peer_sockfd, const struct sockaddr_in &clientAddress);


    int sock_fd{};

    int _acceptClientWrapper(sockaddr_in &clientAddress) const;
};


#endif //RAFT_CPP_TCPSERVER_H
