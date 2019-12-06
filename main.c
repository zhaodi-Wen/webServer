//
// Created by wzd on 19-12-5.
//

#include <stdio.h>
#include "threadpool.h"
#include "http.h"

#define DEFAULT_CONFIG "conf.conf"

#define TRACE_CMH_1 (printf("%s(%d)-<%s>: ",__FILE__, __LINE__, __FUNCTION__), printf)

extern struct epoll_event *events;
char *con_file = DEFAULT_CONFIG;
tk_conf_t conf;


int main(int argc, char *argv[]) {

    //读取文件配置
    read_conf(con_file, &conf);
    //处理SIGPIPE
    handle_for_sigpipe();
    printf("config port %d\n",conf.port);
    printf("config thread num %d\n",conf.thread_num);
    //初始化套接字
    int listen_fd = socket_bind_listen(conf.port);
    printf("listen fd%d\n",listen_fd);
    //设置socket为非阻塞
    int rc = make_socket_non_blocking(listen_fd);

    //创建epoll并且注册文件描述符
    int epoll_fd = tk_epoll_create(0);
    tk_http_request_t *request = (tk_http_request_t *) malloc(sizeof(tk_http_request_t));
    tk_init_request_t(request, listen_fd, epoll_fd, conf.root);
    tk_epoll_add(epoll_fd, listen_fd, request, (EPOLLIN | EPOLLET));
    printf("after  tk_epoll_add\n");


    //初始化线程池
    tk_threadpool_t *tp = threadpool_init(conf.thread_num);
    printf("after  threadpool_init \n");

    //初始化计时器
    tk_timer_init();

    //
    while (1) {
        printf("----------------------------\n");
        //得到最近并且未被删除时间和当前时间的差值(等待时间)
        int time = tk_find_timer();
        printf("after  tk_find_timer \n");

        //调用epoll_wair函数,返回接受时间的数目
        int event_num = tk_epoll_wait(epoll_fd, events, MAXEVENTS, -1);
        printf("after  tk_epoll_wait \n");
        printf("event num %d \n", event_num);


        //处理已经超时的请求
        tk_handle_expire_timers();
        TRACE_CMH_1("SUB: [%d]\n");
        printf("after  tk_handle_expire_timers \n");

        //遍历events数组,根据监听种类和描述符类型进行分发操作
        tk_handle_events(epoll_fd, listen_fd, events, event_num, conf.root, tp);
        printf("after  tk_handle_events \n");

    }
    //回收线程资源
//    threadpool_destroy(tp,graceful_shutdown);
}