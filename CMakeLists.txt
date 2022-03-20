cmake_minimum_required(VERSION 3.21)
project(raft_cpp)

set(CMAKE_CXX_STANDARD 11)

add_executable(raft_cpp server/main.cpp server/TCPServer.cpp server/RaftServer.cpp server/ServerUtils.cpp)