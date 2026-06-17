#ifndef OSAL_TASK_H
#define OSAL_TASK_H
#include "osal_types.h"
osal_task_t osal_task_create(const char *name, osal_task_func_t func,
                              void *param, uint32_t stack_size,
                              osal_priority_t priority);
void osal_task_delete(osal_task_t task);
void osal_task_delay(uint32_t ms);
osal_tick_t osal_task_get_tick_count(void);
void osal_task_notify_give(osal_task_t task);
uint32_t osal_task_notify_take(uint32_t timeout_ms);
void osal_task_suspend(osal_task_t task);
void osal_task_resume(osal_task_t task);
#endif
