/* OSAL POSIX 后端实现 — 用于 PC 开发和测试 */
#include "osal.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ---- 内部任务结构 ---- */
typedef struct {
    pthread_t       thread;
    osal_task_func_t func;
    void           *param;
    char            name[32];
    uint32_t        stack_size;
    osal_priority_t priority;
    bool            running;
    /* Task notification: pthread condition variable + 32-bit value */
    pthread_mutex_t notify_mutex;
    pthread_cond_t  notify_cond;
    uint32_t        notify_value;
    bool            notify_pending;
} posix_task_t;

/* ---- 内部队列结构 ---- */
typedef struct {
    uint8_t  *buffer;
    uint32_t  item_size;
    uint32_t  item_count;
    uint32_t  head;
    uint32_t  tail;
    uint32_t  count;
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} posix_queue_t;

/* ---- 内部信号量结构 ---- */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    uint32_t        count;
    uint32_t        max;
    bool            is_mutex;
    pthread_t       mutex_owner;
} posix_sem_t;

/* ---- 内部事件组结构 ---- */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    uint32_t        bits;
} posix_event_t;

/* ---- 内部定时器结构 ---- */
typedef struct {
    pthread_t       thread;
    char            name[32];
    uint32_t        period_ms;
    bool            auto_reload;
    osal_timer_cb_t callback;
    void           *param;
    bool            running;
    pthread_mutex_t mutex;
} posix_timer_t;

/* ================================================================
 * Task
 * ================================================================ */
static void *posix_task_wrapper(void *arg) {
    posix_task_t *pt = (posix_task_t *)arg;
    pt->func(pt->param);
    pt->running = false;
    return NULL;
}

osal_task_t osal_task_create(const char *name, osal_task_func_t func,
                              void *param, uint32_t stack_size,
                              osal_priority_t priority) {
    (void)stack_size; /* POSIX 下线程栈由系统管理，忽略此参数 */
    posix_task_t *pt = calloc(1, sizeof(posix_task_t));
    if (!pt) return NULL;
    strncpy(pt->name, name, sizeof(pt->name) - 1);
    pt->func = func;
    pt->param = param;
    pt->stack_size = stack_size;
    pt->priority = priority;
    pt->running = true;
    pthread_mutex_init(&pt->notify_mutex, NULL);
    pthread_cond_init(&pt->notify_cond, NULL);
    pt->notify_value = 0;
    pt->notify_pending = false;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&pt->thread, &attr, posix_task_wrapper, pt);
    pthread_attr_destroy(&attr);
    return (osal_task_t)pt;
}

void osal_task_delete(osal_task_t task) {
    if (!task) return;
    posix_task_t *pt = (posix_task_t *)task;
    pt->running = false;
    pthread_join(pt->thread, NULL);
    pthread_mutex_destroy(&pt->notify_mutex);
    pthread_cond_destroy(&pt->notify_cond);
    free(pt);
}

void osal_task_delay(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

osal_tick_t osal_task_get_tick_count(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (osal_tick_t)ts.tv_sec * 1000000ULL + (osal_tick_t)ts.tv_nsec / 1000ULL;
}

void osal_task_notify_give(osal_task_t task) {
    if (!task) return;
    posix_task_t *pt = (posix_task_t *)task;
    pthread_mutex_lock(&pt->notify_mutex);
    pt->notify_value++;
    pt->notify_pending = true;
    pthread_cond_signal(&pt->notify_cond);
    pthread_mutex_unlock(&pt->notify_mutex);
}

uint32_t osal_task_notify_take(uint32_t timeout_ms) { (void)timeout_ms;
    /* 取当前线程对应的 task (简化: 不支持跨线程通知给"自己") */
    /* 实际使用时需要在任务创建时保存 pt 到 TLS */
    return 0; /* TODO: 完善 TLS 支持 */
}

void osal_task_suspend(osal_task_t task) { (void)task; }
void osal_task_resume(osal_task_t task) { (void)task; }

/* ================================================================
 * Queue
 * ================================================================ */
osal_queue_t osal_queue_create(uint32_t item_size, uint32_t item_count) {
    posix_queue_t *pq = calloc(1, sizeof(posix_queue_t));
    if (!pq) return NULL;
    pq->buffer = calloc(item_count, item_size);
    if (!pq->buffer) { free(pq); return NULL; }
    pq->item_size = item_size;
    pq->item_count = item_count;
    pthread_mutex_init(&pq->mutex, NULL);
    pthread_cond_init(&pq->not_empty, NULL);
    pthread_cond_init(&pq->not_full, NULL);
    return (osal_queue_t)pq;
}

int osal_queue_send(osal_queue_t q, const void *item, uint32_t timeout_ms) {
    if (!q || !item) return OSAL_PARAM_ERR;
    posix_queue_t *pq = (posix_queue_t *)q;
    pthread_mutex_lock(&pq->mutex);
    while (pq->count >= pq->item_count && timeout_ms > 0) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (pthread_cond_timedwait(&pq->not_full, &pq->mutex, &ts) != 0) {
            pthread_mutex_unlock(&pq->mutex);
            return OSAL_TIMEOUT;
        }
    }
    if (pq->count >= pq->item_count) {
        pthread_mutex_unlock(&pq->mutex);
        return OSAL_ERROR;
    }
    memcpy(pq->buffer + pq->tail * pq->item_size, item, pq->item_size);
    pq->tail = (pq->tail + 1) % pq->item_count;
    pq->count++;
    pthread_cond_signal(&pq->not_empty);
    pthread_mutex_unlock(&pq->mutex);
    return OSAL_OK;
}

int osal_queue_receive(osal_queue_t q, void *item, uint32_t timeout_ms) {
    if (!q || !item) return OSAL_PARAM_ERR;
    posix_queue_t *pq = (posix_queue_t *)q;
    pthread_mutex_lock(&pq->mutex);
    while (pq->count == 0 && timeout_ms > 0) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (pthread_cond_timedwait(&pq->not_empty, &pq->mutex, &ts) != 0) {
            pthread_mutex_unlock(&pq->mutex);
            return OSAL_TIMEOUT;
        }
    }
    if (pq->count == 0) {
        pthread_mutex_unlock(&pq->mutex);
        return OSAL_ERROR;
    }
    memcpy(item, pq->buffer + pq->head * pq->item_size, pq->item_size);
    pq->head = (pq->head + 1) % pq->item_count;
    pq->count--;
    pthread_cond_signal(&pq->not_full);
    pthread_mutex_unlock(&pq->mutex);
    return OSAL_OK;
}

void osal_queue_delete(osal_queue_t q) {
    if (!q) return;
    posix_queue_t *pq = (posix_queue_t *)q;
    pthread_mutex_destroy(&pq->mutex);
    pthread_cond_destroy(&pq->not_empty);
    pthread_cond_destroy(&pq->not_full);
    free(pq->buffer);
    free(pq);
}

/* ================================================================
 * Semaphore / Mutex
 * ================================================================ */
osal_sem_t osal_sem_create_binary(void)  { return osal_sem_create_counting(1, 0); }
osal_sem_t osal_sem_create_counting(uint32_t max, uint32_t initial) {
    posix_sem_t *ps = calloc(1, sizeof(posix_sem_t));
    if (!ps) return NULL;
    pthread_mutex_init(&ps->mutex, NULL);
    pthread_cond_init(&ps->cond, NULL);
    ps->max = max;
    ps->count = initial;
    return (osal_sem_t)ps;
}
int osal_sem_take(osal_sem_t sem, uint32_t timeout_ms) {
    if (!sem) return OSAL_PARAM_ERR;
    posix_sem_t *ps = (posix_sem_t *)sem;
    pthread_mutex_lock(&ps->mutex);
    while (ps->count == 0) {
        if (timeout_ms == 0) { pthread_mutex_unlock(&ps->mutex); return OSAL_TIMEOUT; }
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (pthread_cond_timedwait(&ps->cond, &ps->mutex, &ts) != 0) {
            pthread_mutex_unlock(&ps->mutex);
            return OSAL_TIMEOUT;
        }
    }
    if (ps->count > 0) ps->count--;
    pthread_mutex_unlock(&ps->mutex);
    return OSAL_OK;
}
int osal_sem_give(osal_sem_t sem) {
    if (!sem) return OSAL_PARAM_ERR;
    posix_sem_t *ps = (posix_sem_t *)sem;
    pthread_mutex_lock(&ps->mutex);
    if (ps->count < ps->max) { ps->count++; pthread_cond_signal(&ps->cond); }
    pthread_mutex_unlock(&ps->mutex);
    return OSAL_OK;
}
void osal_sem_delete(osal_sem_t sem) {
    if (!sem) return;
    posix_sem_t *ps = (posix_sem_t *)sem;
    pthread_mutex_destroy(&ps->mutex);
    pthread_cond_destroy(&ps->cond);
    free(ps);
}
osal_sem_t osal_mutex_create(void) {
    posix_sem_t *ps = (posix_sem_t *)osal_sem_create_counting(1, 1);
    if (ps) ps->is_mutex = true;
    return (osal_sem_t)ps;
}
int osal_mutex_lock(osal_sem_t mutex)   { return osal_sem_take(mutex, 0xFFFFFFFF); }
int osal_mutex_unlock(osal_sem_t mutex) { return osal_sem_give(mutex); }

/* ================================================================
 * Event Group
 * ================================================================ */
osal_event_t osal_event_group_create(void) {
    posix_event_t *pe = calloc(1, sizeof(posix_event_t));
    if (!pe) return NULL;
    pthread_mutex_init(&pe->mutex, NULL);
    pthread_cond_init(&pe->cond, NULL);
    return (osal_event_t)pe;
}
uint32_t osal_event_group_wait(osal_event_t eg, uint32_t bits,
                                bool clear, bool wait_all, uint32_t timeout_ms) {
    if (!eg) return 0;
    posix_event_t *pe = (posix_event_t *)eg;
    pthread_mutex_lock(&pe->mutex);
    while (1) {
        uint32_t matched = pe->bits & bits;
        bool done = wait_all ? (matched == bits) : (matched != 0);
        if (done) {
            if (clear) pe->bits &= ~matched;
            pthread_mutex_unlock(&pe->mutex);
            return matched;
        }
        if (timeout_ms == 0) { pthread_mutex_unlock(&pe->mutex); return 0; }
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (pthread_cond_timedwait(&pe->cond, &pe->mutex, &ts) != 0) {
            pthread_mutex_unlock(&pe->mutex);
            return 0;
        }
    }
}
int osal_event_group_set(osal_event_t eg, uint32_t bits) {
    if (!eg) return OSAL_PARAM_ERR;
    posix_event_t *pe = (posix_event_t *)eg;
    pthread_mutex_lock(&pe->mutex);
    pe->bits |= bits;
    pthread_cond_broadcast(&pe->cond);
    pthread_mutex_unlock(&pe->mutex);
    return OSAL_OK;
}
void osal_event_group_delete(osal_event_t eg) {
    if (!eg) return;
    posix_event_t *pe = (posix_event_t *)eg;
    pthread_mutex_destroy(&pe->mutex);
    pthread_cond_destroy(&pe->cond);
    free(pe);
}

/* ================================================================
 * Timer (简化: POSIX 下用独立线程实现)
 * ================================================================ */
static void *posix_timer_thread(void *arg) {
    posix_timer_t *pt = (posix_timer_t *)arg;
    while (pt->running) {
        usleep((useconds_t)pt->period_ms * 1000);
        if (pt->running && pt->callback) pt->callback(pt->param);
        if (!pt->auto_reload) break;
    }
    return NULL;
}
osal_timer_t osal_timer_create(const char *name, uint32_t period_ms,
                                bool auto_reload, osal_timer_cb_t cb, void *param) {
    posix_timer_t *pt = calloc(1, sizeof(posix_timer_t));
    if (!pt) return NULL;
    strncpy(pt->name, name, sizeof(pt->name) - 1);
    pt->period_ms = period_ms;
    pt->auto_reload = auto_reload;
    pt->callback = cb;
    pt->param = param;
    pt->running = false;
    pthread_mutex_init(&pt->mutex, NULL);
    return (osal_timer_t)pt;
}
int osal_timer_start(osal_timer_t timer) {
    if (!timer) return OSAL_PARAM_ERR;
    posix_timer_t *pt = (posix_timer_t *)timer;
    pt->running = true;
    pthread_create(&pt->thread, NULL, posix_timer_thread, pt);
    return OSAL_OK;
}
int osal_timer_stop(osal_timer_t timer) {
    if (!timer) return OSAL_PARAM_ERR;
    posix_timer_t *pt = (posix_timer_t *)timer;
    pt->running = false;
    pthread_join(pt->thread, NULL);
    return OSAL_OK;
}
void osal_timer_delete(osal_timer_t timer) {
    if (!timer) return;
    posix_timer_t *pt = (posix_timer_t *)timer;
    if (pt->running) osal_timer_stop(timer);
    pthread_mutex_destroy(&pt->mutex);
    free(pt);
}
