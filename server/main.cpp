
#include "RaftServer.h"

int main(int argc, char** argv){
//    atoi(argv[1])
    if(argc !=3 ){
        printf("USEAGE: ./server <server_name> <hostname/ip>\n");
        return 1;
    }

    RaftServer server(0, argv[2], "3000");

    char *nodes[] = {"node0\n", "node1\n", "node2\n"};
    for(char *node: nodes){
        if(strncmp(argv[2], node, 5) != 0){
            server.lazyConnect(node);
        }
    }
    server.actAsServer();
}