//
// Created by wzd on 19-12-2.
//

#ifndef CSERVER_EPOLL_H
#define CSERVER_EPOLL_H

#include <sys/epoll.h>
#include "http.h"
#include "threadpool.h"


#define MAXEVENTS 1024   //最多连接数
int tk_epoll_create(int flags);
int tk_epoll_add(int epoll_fd,int fd,tk_http_request_t *request,int event);
int tk_epoll_mod(int epoll_fd,int fd,tk_http_request_t *request,int event);
int tk_epoll_del(int epoll_fd,int fd,tk_http_request_t *request,int event);
int tk_epoll_wait(int epoll_fd,struct epoll_event *events,int max_event,int timeouts);
void tk_handle_events(int epoll_fd,int listen_fd, struct epoll_event* event,
                      int events_num,char* path,tk_threadpool_t*tp);
#endif //CSERVER_EPOLL_H
