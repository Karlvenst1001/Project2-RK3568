#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "common.h"

// 任务结构
typedef struct task
{
    void (*function)(void *arg); // 任务函数指针
    void *arg;                   // 任务参数
    struct task *next;           // 下一个任务
} task_t;

// 线程池结构
typedef struct thread_pool
{
    pthread_mutex_t lock; // 互斥锁
    pthread_cond_t cond;  // 条件变量s
    task_t *task_head;    // 任务队列头
    task_t *task_tail;    // 任务队列尾
    pthread_t *threads;   // 线程数组
    int thread_count;     // 线程数量
    int task_count;       // 任务数量
    bool shutdown;        // 关闭标志
} thread_pool_t;

void *worker_thread(void *arg);                                                      // 工作线程函数
thread_pool_t *thread_pool_create(int thread_count);                                 // 创建线程池
bool thread_pool_add_task(thread_pool_t *pool, void (*function)(void *), void *arg); // 向线程池添加任务
void thread_pool_destroy(thread_pool_t *pool);                                       // 销毁线程池

#endif
