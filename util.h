//
// Created by wzd on 19-12-2.
//

#ifndef CSERVER_UTIL_H
#define CSERVER_UTIL_H

#define PATHLEN 128
#define LISTENQ 1024
#define BUFLEN  8192
#define DELIM  "="

#define TK_CONF_OK 0
#define TK_CONF_ERROR -1

#define MIN(a,b) ((a)<(b) ? (a):(b))

typedef struct tk_conf{
    char root[PATHLEN];
    int port;
    int thread_num;
}tk_conf_t;

//读取配置
int read_conf(char *filename,tk_conf_t *conf);

void handle_for_sigpipe();
//绑定监听
int socket_bind_listen(int port);
int make_socket_non_blocking(int fd);
//处理连接
void accept_connection(int listen_fd,int epoll_fd, char *path);
#endif //CSERVER_UTIL_H
