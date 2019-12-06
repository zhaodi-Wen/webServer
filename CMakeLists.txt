cmake_minimum_required(VERSION 3.13)
project(c_server)

set(CMAKE_CXX_STANDARD 14)

add_executable(c_server util.c util.h epoll.h epoll.c http_request.h http_request.c list.h threadpool.h threadpool.c http.c http.h timer.c timer.h priority_queue.c priority_queue.h rio.c rio.h http_parse.c http_parse.h error.h main.c)