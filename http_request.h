//
// Created by wzd on 19-12-2.
//

#ifndef CSERVER_HTTP_REQUEST_H
#define CSERVER_HTTP_REQUEST_H

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "list.h"

#define TK_AGAIN EAGAIN
#define TK_HTTP_PARSE_INVALID_METHOD         10
#define TK_HTTP_PARSE_INVALID_REQUEST        11
#define TK_HTTP_PARSE_INVALID_HEADER         12

#define TK_HTTP_UNKNOWN                      0X0001
#define TK_HTTP_GET                          0X0002
#define TK_HTTP_HEAD                         0X0004
#define TK_HTTP_POST                         0X0008

#define TK_HTTP_OK                           200
#define TK_HTTP_NOT_MODIFIED                 304
#define TK_HTTP_NOT_FOUND                    404
#define MAX_BUF   8124


//请求头的结构
typedef struct tk_http_request{
    char *root;             //配置目录
    int fd;                 //文件描述符,可用去监听或者连接
    int epoll_fd;           //文件描述符句柄,用于控制
    char buf[MAX_BUF];      //用户的缓冲,明确来说是客户端的缓冲
    size_t pos;             //
    size_t last;
    int state;              //请求解析头的方法

    void* request_start;
    void* method_end;
    int method;             //使用的方法
    void* uri_start;
    void* uri_end;
    void* path_start;
    void* path_end;
    void* query_start;
    void* query_end;
    int http_major;
    int http_minor;
    void* request_end;

    struct list_head list;//存储请求头,list.h中定义了

    void* cur_header_key_start;
    void* cur_header_key_end;
    void* cur_header_value_start;
    void* cur_header_value_end;
    void* timer;                //指向了时间戳的结构
}tk_http_request_t;

//响应头的结构
typedef struct tk_http_out{
    int fd;         //连接的文件描述符
    int keep_alive; //HTTP状态,类似于http1.0和http1.1
    time_t mtime;   //文件类型
    int modified;   //是否修改了
    int status;     //服务端的返回码
}tk_http_out_t;


typedef struct tk_http_header{
    void *key_start;
    void *key_end;
    void *value_start;
    void *value_end;
    struct list_head list;
}tk_http_header_t;

typedef int(*tk_http_header_handler_pt)(tk_http_request_t *request,tk_http_out_t*out, char *data,int len);

typedef struct tk_http_header_handle{
    char *name;
    tk_http_header_handler_pt handler;//函数指针
}tk_http_header_handle_t;

extern tk_http_header_handle_t tk_http_headers_in[];

void tk_http_handle_header(tk_http_request_t *request,tk_http_out_t *out);
int tk_http_close_conn(tk_http_request_t * request);
int tk_init_request_t(tk_http_request_t *request,int fd,int epoll_fd,char *path);
int tk_init_out_t(tk_http_out_t *out,int fd);
const char *get_shortmsg_from_status_code(int status_code);
#endif //CSERVER_HTTP_REQUEST_H
