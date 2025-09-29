#include "thread_pool.h"

/**
 * @brief 工作线程
 * @param arg 线程池指针传参
 */
void *worker_thread(void *arg)
{
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1)
    {
        pthread_mutex_lock(&pool->lock);
        // 检查是否需要关闭线程
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }

        // 获取任务
        task_t *task = pool->task_head;
        if (task == (task_t *)NULL)
        {
            pthread_mutex_unlock(&pool->lock);
            continue;
        }

        // 从队列中移除任务
        pool->task_head = task->next;
        if (pool->task_head == (task_t *)NULL)
        {
            pool->task_tail = (task_t *)NULL;
        }
        // 队列任务数量-1
        pool->task_count--;

        pthread_mutex_unlock(&pool->lock);

        // 执行任务
        (*(task->function))(task->arg);

        // 释放任务内存
        free(task);
    }
}

/**
 * @brief 创建线程池
 * @param thread_count  创建线程数量
 * @return 线程池指针
 * @return NULL 创建失败
 */
thread_pool_t *thread_pool_create(int thread_count)
{
    if (thread_count <= 0)
    {
        thread_count = 5; // 默认4个线程
    }

    thread_pool_t *pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if (pool == (thread_pool_t *)NULL)
    {
        perror("create thread pool falied:");
        return (thread_pool_t *)NULL;
    }

    // 初始化线程池字段
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->thread_count = thread_count;
    pool->task_count = 0;
    pool->task_head = (task_t *)NULL;
    pool->task_tail = (task_t *)NULL;
    pool->shutdown = false;

    // 初始化互斥锁和条件变量
    if (pthread_mutex_init(&pool->lock, NULL) != 0 || pthread_cond_init(&pool->cond, NULL) != 0)
    {
        free(pool->threads);
        free(pool);
        perror("pthread_mutex_init or pthread_cond_init falied:");
        return NULL;
    }

    // 创建工作线程
    for (int i = 0; i < thread_count; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0)
        {
            perror("pthread_create failed:");
            thread_pool_destroy(pool);
            return (thread_pool_t *)NULL;
        }
    }

    return pool;
}

/**
 * @brief 向线程池添加任务
 * @param pool 线程池指针
 * @param function 任务函数指针
 * @param arg 任务函数参数
 * @return true 添加成功
 * @return false 添加失败
 */
bool thread_pool_add_task(thread_pool_t *pool, void (*function)(void *), void *arg)
{
    if (pool == NULL || function == NULL)
    {
        return false;
    }

    // 创建新任务
    task_t *task = (task_t *)malloc(sizeof(task_t));
    if (task == (task_t *)NULL)
    {
        perror("malloc failed:");
        return false;
    }

    task->function = function;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->lock);

    // 将任务添加到队列尾部
    if (pool->task_head == NULL)
    {
        pool->task_head = task;
    }
    else
    {
        pool->task_tail->next = task;
    }
    pool->task_tail = task;
    pool->task_count++;

    // 通知工作线程有新任务
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->lock);

    return true;
}

/**
 * @brief 销毁线程池
 * @param pool 线程池指针
 * @return -1 销毁失败
 */
void thread_pool_destroy(thread_pool_t *pool)
{
    if (pool == (thread_pool_t *)NULL)
    {
        return (void)-1;
    }

    pthread_mutex_lock(&pool->lock);
    pool->shutdown = true;
    pthread_mutex_unlock(&pool->lock);

    // 唤醒所有工作线程
    pthread_cond_broadcast(&pool->cond);

    // 等待所有工作线程退出
    for (int i = 0; i < pool->thread_count; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }

    // 释放剩余任务
    task_t *task = pool->task_head;
    while (task != (task_t *)NULL)
    {
        task_t *next = task->next;
        free(task);
        task = next;
    }

    // 释放资源
    free(pool->threads);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    free(pool);

    printf("Thread pool destroyed\n");
}

