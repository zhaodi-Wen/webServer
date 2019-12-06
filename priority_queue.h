//
// Created by wzd on 19-12-4.
//

#ifndef CSERVER_PRIORITY_QUEUE_H
#define CSERVER_PRIORITY_QUEUE_H

#include <stdlib.h>

#define TK_PQ_DEFAULT_SIZE 10

typedef int (*tk_pq_comparator_pt)(void *pi, void*qj);


//优先队列的结构
typedef struct priority_queue{
    void **pq;      //优先队列的指针
    size_t nalloc;  //队列的实际元素的个数
    size_t size;    //队列的大小
    tk_pq_comparator_pt comp;   //这个就是个比较函数,后面有一个比较,用于将队列使用堆的形式进行排序,找出最小的值,是一个最小堆
}tk_pq_t;


int tk_pq_init(tk_pq_t* tk_pq,tk_pq_comparator_pt comp,size_t size);
int tk_pq_is_empty(tk_pq_t *tk_pq);
size_t tk_pq_size(tk_pq_t *tk_pq);
void *tk_pq_min(tk_pq_t *tk_pq);
int tk_pq_delmin(tk_pq_t*tk_pq);
int tk_pq_insert(tk_pq_t*tk_pq, void *item);
int tk_pq_sink(tk_pq_t *tk_pq,size_t i);
#endif //CSERVER_PRIORITY_QUEUE_H
