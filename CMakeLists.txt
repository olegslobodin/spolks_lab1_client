cmake_minimum_required(VERSION 3.6)
project(spolks_lab1_client)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES spolks_lab1_client/Source.cpp)
add_executable(spolks_lab1_client ${SOURCE_FILES})