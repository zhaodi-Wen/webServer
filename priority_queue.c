//
// Created by wzd on 19-12-4.
//

#include "priority_queue.h"
#include <stdlib.h>
#include <string.h>
#include <printf.h>
#include <stdio.h>

//此宏展开后，类似于printf("123"),printf("456");
#define TRACE_CMH_1 (printf("%s(%d)-<%s>: ",__FILE__, __LINE__, __FUNCTION__), printf)
//交换优先队列的两个元素
void exch(tk_pq_t *tk_pq, size_t i, size_t j) {
    void *tmp = tk_pq->pq[i];
    tk_pq->pq[i] = tk_pq->pq[j];
    tk_pq->pq[j] = tmp;
}


//所谓的上浮,就是从下面往上面比较和交换,堆排序的fixUp函数
void swim(tk_pq_t *tk_pq, size_t k) {
    while (k > 1 && tk_pq->comp(tk_pq->pq[k], tk_pq->pq[k / 2])) {
        exch(tk_pq, k, k / 2);
        k /= 2;
    }
}

//从上到下的堆排序,所谓的下沉,类比与之前的最小堆的fixdown算法
int sink(tk_pq_t *tk_pq, size_t k) {
    size_t j;
    size_t nalloc = tk_pq->nalloc;
    while ((k << 1) <= nalloc) {
        j = k << 1;
        if ((j < nalloc) && (tk_pq->comp(tk_pq->pq[j + 1], tk_pq->pq[j])))
            j++;//找到子节点中最大的那个
        if (!tk_pq->comp(tk_pq->pq[j], tk_pq->pq[k]))//如果子节点比比较的节点的节点大,没必要比较了,说明k节点是已经交换到父节点的位置上了
            break;
        exch(tk_pq, j, k);
        k = j;
    }
    return k;
}

int tk_pq_sink(tk_pq_t *tk_pq, size_t i) {
    return sink(tk_pq, i);
}


int tk_pq_init(tk_pq_t *tk_pq, tk_pq_comparator_pt comp, size_t size) {
    //为tk_pq_t节点的pq分配void*指针
    tk_pq->pq = (void **) malloc(sizeof(void *) * (size + 1));

    if (!tk_pq->pq)
        return -1;

    tk_pq->nalloc = 0;
    tk_pq->size = size + 1;
    tk_pq->comp = comp;

    return 0;
}

int tk_pq_is_empty(tk_pq_t *tk_pq) {
    return (tk_pq->nalloc == 0) ? 1 : 0;
}

size_t tk_pq_size(tk_pq_t *tk_pq) {
    //获取优先队列的大小
    return tk_pq->nalloc;
}


void *tk_pq_min(tk_pq_t *tk_pq) {
    //优先队列最小值直接返回第一个元素的指针
    if (tk_pq_is_empty(tk_pq))
        return (void *) (-1);

    return tk_pq->pq[1];
}

int resize(tk_pq_t *tk_pq, size_t new_size) {
    if (new_size <= tk_pq->nalloc)
        return -1;

    void **new_ptr = (void *) malloc(sizeof(void *) * (new_size));

    if (!new_ptr)
        return -1;


    //将原本的nmalloc+1个元素拷贝到new_ptr指向的位置
    memcpy(new_ptr, tk_pq->pq, sizeof(void *) * (tk_pq->nalloc + 1));

    //释放旧元素
    free(tk_pq->pq);

    //重新改写优先队列元素pq指针为new_ptr;
    tk_pq->pq = new_ptr;
    tk_pq->size = new_size;
    return 0;
}

int tk_pq_delmin(tk_pq_t *tk_pq) {
    printf("in tk_pq_delmin\n");
    if (tk_pq_is_empty(tk_pq))
        return 0;
    printf("%zu \n", tk_pq->nalloc);

    exch(tk_pq, 1, tk_pq->nalloc);
    printf("after exch\n");
    --tk_pq->nalloc;
    sink(tk_pq, 1);
    if ((tk_pq->nalloc > 0) && (tk_pq->nalloc <= (tk_pq->size - 1) / 4)) {
        if (resize(tk_pq, tk_pq->size / 2) < 0)
            return -1;
    }
    return 0;
}


int tk_pq_insert(tk_pq_t *tk_pq, void *item) {
    if (tk_pq->nalloc + 1 == tk_pq->size) {
        if (resize(tk_pq, tk_pq->size * 2) < 0)
            return -1;
    }

    tk_pq->pq[++tk_pq->nalloc] = item;
    swim(tk_pq, tk_pq->nalloc);
    return 0;
}
