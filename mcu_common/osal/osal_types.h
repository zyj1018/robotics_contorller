/* OSAL 类型定义 */
#ifndef OSAL_TYPES_H
#define OSAL_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef OSAL_BACKEND_FREERTOS
  #include "FreeRTOS.h"
  typedef TickType_t          osal_tick_t;
  typedef TaskHandle_t        osal_task_t;
  typedef QueueHandle_t       osal_queue_t;
  typedef SemaphoreHandle_t   osal_sem_t;
  typedef EventGroupHandle_t  osal_event_t;
  typedef TimerHandle_t       osal_timer_t;
#else
  #include <pthread.h>
  #include <stdint.h>
  typedef uint64_t  osal_tick_t;
  typedef void *    osal_task_t;
  typedef void *    osal_queue_t;
  typedef void *    osal_sem_t;
  typedef void *    osal_event_t;
  typedef void *    osal_timer_t;
#endif

typedef void (*osal_task_func_t)(void *param);

typedef enum {
    OSAL_PRIORITY_IDLE      = 0,
    OSAL_PRIORITY_LOW       = 1,
    OSAL_PRIORITY_NORMAL    = 2,
    OSAL_PRIORITY_HIGH      = 3,
    OSAL_PRIORITY_REALTIME  = 4,
    OSAL_PRIORITY_CRITICAL  = 5,
} osal_priority_t;

#define OSAL_OK          0
#define OSAL_TIMEOUT    -1
#define OSAL_ERROR      -2
#define OSAL_PARAM_ERR  -3

#endif /* OSAL_TYPES_H */
