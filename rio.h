//
// Created by wzd on 19-12-4.
//

#ifndef CSERVER_RIO_H
#define CSERVER_RIO_H

#include <sys/types.h>
#define RIO_BUFSIZE 8192

//io包的结构
typedef struct {
    int rio_fd;            //文件描述符
    ssize_t  rio_cnt;      //还未读取的内部的缓存buf大小
    char *rio_bufptr;      //下一个还未读取的内部缓存的指针
    char rio_buf[RIO_BUFSIZE];   //内部缓存
}rio_t;


ssize_t  rio_readn(int fd, void *usrbuf,size_t n);
ssize_t  rio_writen(int fd, void *usrbuf,size_t n);
void rio_readinitb(rio_t *rp,int fd);
ssize_t rio_readnb(rio_t *rp, void *usrbuf,size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf,size_t maxlen);
#endif //CSERVER_RIO_H
