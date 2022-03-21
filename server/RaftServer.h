//--------------------------------------
// Event Loop
//--------------------------------------
// https://github.com/goraft/raft/blob/0061b6c82526bd96292f41fa72358ec13ee3853c/server.go
//               ________
//            --|Snapshot|                 timeout
//            |  --------                  ______
// recover    |       ^                   |      |
// snapshot / |       |snapshot           |      |
// higher     |       |                   v      |     recv majority votes
// term       |    --------    timeout    -----------                        -----------
//            |-> |Follower| ----------> | Candidate |--------------------> |  Leader   |
//                 --------               -----------                        -----------
//                    ^          higher term/ |                         higher term |
//                    |            new leader |                                     |
//                    |_______________________|____________________________________ |

#ifndef RAFT_CPP_RAFTSERVER_H
#define RAFT_CPP_RAFTSERVER_H
#include "TCPServer.h"
#include "ServerUtils.h"

#include <cstdio>

#include <vector>
#include <cstring>
#include <string>
#include <unistd.h>
#include <map>
#include <string>
#include <queue>

constexpr int TOT_SERVER = 5;

struct RaftPayload{
    /*
     * Type{ 1: RequestVoteRPC,
     *       2: AppendEntriesRPC
     *       3: InstallSnapshotRPC,}
     *
     * The payload is implemented each payload has relevant fields for all
     * the RPC bases on the type(in this implementation an integer)
     * data is parsed accordingly */
    int type{};


    // RequestVote
    int term{};
    int candidateId{};
    char data[1024]{};
    int lastLogIndex{};
    int lastLogTerm{};

    // Results

};

enum State{
    Initializing = 0,
    Follower = 1,
    Candidate = 2,
    Leader = 3,
};

class RaftServer:  public TCPServer{
private:
    int nodeId;
    int totalEstablishedConn;

    std::map<std::string, int> connectedPeer;
    std::map<int, std::string> monitoredPeer;


    std::deque<std::string> toConnect;

    State s;
    void postConnectRoutine(int commSock,const struct sockaddr_in &clientAddress) final;
    void preCloseRoutine(int commSock) final ;
    int _handleRequest(int clientSock) override ;
    void timeoutEvent() override;


public:
    RaftServer(int nodeId, const char *hostname, const char *port) :
            TCPServer(hostname, port), nodeId(nodeId), s(Initializing), totalEstablishedConn(0)
    {}

    bool descriptorEvents(fd_set &readfds, int i) override;
    bool tryConnecting(const char *host);
    void lazyConnect(const char *host);

    bool isServerRequest(int fd);

};


#endif //RAFT_CPP_RAFTSERVER_H
