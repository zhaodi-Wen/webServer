//
// Created by wzd on 19-12-2.
//

#ifndef CSERVER_THREADPOOL_H
#define CSERVER_THREADPOOL_H

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>


//任务结构
typedef struct tk_task{
    //可以想到pthread_t(&thread,&fun,arg);的定义
    void (*func)(void*);    //任务函数
    void *arg;          //任务的函数的参数
    struct tk_task *next;       //任务链表(下一节点指针)
}tk_task_t;

typedef struct threadpool{//线程池
    pthread_mutex_t lock;   //互斥锁
    pthread_cond_t cond;    //条件变量
    pthread_t *pthreads;     //线程
    tk_task_t *head;        //任务链表
    int thread_count;       //线程数
    int queue_size ;        //任务链表长
    int shutdown;           //关机模式
    int started;
}tk_threadpool_t;

typedef enum{
    tk_tp_invalid = -1,
    tk_tp_lock_fail = -2,
    tk_tp_already_shutdown = -3,
    tk_tp_cond_broadcast = -4,
    tk_tp_thread_fail = -5,
}tk_threadpool_error_t;

typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
}tk_threadpool_sd_t;

tk_threadpool_t* threadpool_init(int thread_num);
int threadpool_add(tk_threadpool_t *pool, void(*func)(void *), void *arg);
int threadpool_destroy(tk_threadpool_t *pool,int graceful);

#endif //CSERVER_THREADPOOL_H
