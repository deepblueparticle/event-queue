cmake_minimum_required(VERSION 3.9)
project(event_queue)

set(CMAKE_CXX_STANDARD 14)
add_executable(event_queue_demo event_queue.h event_queue.cpp demo.cpp)
