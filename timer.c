//
// Created by wzd on 19-12-3.
//

#include "timer.h"
#include <sys/time.h>
//此宏展开后，类似于printf("123"),printf("456");
#define TRACE_CMH_1 (printf("%s(%d)-<%s>: ",__FILE__, __LINE__, __FUNCTION__), printf)
tk_pq_t tk_timer;
size_t tk_current_msec;

//
int timer_comp(void *ti, void *tj) {
    tk_timer_t *timeri = (tk_timer_t *) ti;
    tk_timer_t *timerj = (tk_timer_t *) tj;
    return (timeri->key < timerj->key) ? 1 : 0;
}


void tk_time_update() {
    struct timeval tv;
    int tc = gettimeofday(&tv, NULL);
    tk_current_msec = ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
    printf("time %zu\n",tk_current_msec);
}

int tk_timer_init() {
    //建立连接后立即初始化
    //初始化优先队列大小TK_PQ_DEFAULT_SIZE = 10;
    int rc = tk_pq_init(&tk_timer, timer_comp, TK_PQ_DEFAULT_SIZE);

    tk_time_update();
    return 0;
}

int tk_find_timer() {
    printf("in tk_find_timer\n");
    int time;
    printf("tk_timer size %zu\n",tk_timer.nalloc);
    while (!tk_pq_is_empty(&tk_timer)) {
        //更新当前时间
        tk_time_update();
        //timer_node指向最小时间
        tk_timer_t *timer_node = (tk_timer_t *) tk_pq_min(&tk_timer);
        printf("after tk_pq_min \n");
        //如果已删除则释放该节点(tk_del_timer只置位不删除)
        if(!timer_node)
            printf("timer node not exists\n");
        if (timer_node->deleted) {
            printf("in if\n");
            int rc = tk_pq_delmin(&tk_timer);
            free(timer_node);
            continue;
        }
        printf("after if\n");
        //此时timer_node为时间的最小节点,time为优先队列中最小时间减去当前时间
        time = (int) (timer_node->key - tk_current_msec);
        time = (time > 0) ? time : 0;
        break;
    }
    return time;

}


void tk_handle_expire_timers() {
    while (!tk_pq_is_empty(&tk_timer)) {
        tk_time_update();
        tk_timer_t *timer_node = (tk_timer_t *) tk_pq_min(&tk_timer);

        //如果节点已经删除
        if (timer_node->deleted) {
            int rc = tk_pq_delmin(&tk_timer);
            free(timer_node);
            continue;
        }

        //最早入队列节点超时时间大于当前时间(未超时)
        //结束超时检查,顺带删了下标记为删除的节点
        if (timer_node->key > tk_current_msec) {
            return;
        }
        int rc = tk_pq_delmin(&tk_timer);
        free(timer_node);
    }
}

void tk_add_timer(tk_http_request_t *request, size_t timeout, timer_handler_pt handler) {
    tk_time_update();
    //申请新的tk_timer_t节点,并加入tk_http_request_t的timer下
    tk_timer_t *timer_node = (tk_timer_t *) malloc(sizeof(tk_timer_t));
    request->timer = timer_node;

    //加入设置超时阀值,删除信息等
    timer_node->key = tk_current_msec + timeout;
    timer_node->deleted = 0;
    timer_node->handler = handler;

    //需要在tk_timer_t节点中反向设置指向对应request的指针
    timer_node->request = request;
    int rc = tk_pq_insert(&tk_timer, timer_node);

}

void tk_del_timer(tk_http_request_t *request) {
    tk_time_update();

    tk_timer_t *timer_node = request->timer;

    //惰性删除
    //标记为已经删除的,在find_timer和handle_expire_timers中检查
    timer_node->deleted = 1;
}