#ifndef OSAL_EVENT_H
#define OSAL_EVENT_H
#include "osal_types.h"
#define OSAL_EVENT_BIT(n) (1UL << (n))
osal_event_t osal_event_group_create(void);
uint32_t osal_event_group_wait(osal_event_t eg, uint32_t bits,
                                bool clear, bool wait_all, uint32_t timeout_ms);
int osal_event_group_set(osal_event_t eg, uint32_t bits);
void osal_event_group_delete(osal_event_t eg);
#endif
