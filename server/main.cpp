#include <cstdio>
#include "RaftServer.h"

//#define HOST_BUILD

int main(int argc, char** argv){
//    atoi(argv[1])
    if(argc !=4 ){
        printf("USEAGE: ./server <server_name> <hostname/ip> <port no>\n");
        return 1;
    }

    RaftServer server(0, argv[2], argv[3]);
#ifndef HOST_BUILD
    std::string nodes[] = {"node0", "node1", "node2"};
    for(std::string &node: nodes){
        if(strncmp(argv[2], node.c_str(), 5) != 0){
            ServerUtils::addServerToListOfKnowHost(node.c_str());
            server.lazyConnect(node.c_str());
        }
    }
#endif //HOST_BUILD
    server.actAsServer();
}