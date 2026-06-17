#ifndef OSAL_TIMER_H
#define OSAL_TIMER_H
#include "osal_types.h"
typedef void (*osal_timer_cb_t)(void *param);
osal_timer_t osal_timer_create(const char *name, uint32_t period_ms,
                                bool auto_reload, osal_timer_cb_t cb, void *param);
int osal_timer_start(osal_timer_t timer);
int osal_timer_stop(osal_timer_t timer);
void osal_timer_delete(osal_timer_t timer);
#endif
