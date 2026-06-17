#ifndef OSAL_QUEUE_H
#define OSAL_QUEUE_H
#include "osal_types.h"
osal_queue_t osal_queue_create(uint32_t item_size, uint32_t item_count);
int osal_queue_send(osal_queue_t q, const void *item, uint32_t timeout_ms);
int osal_queue_receive(osal_queue_t q, void *item, uint32_t timeout_ms);
void osal_queue_delete(osal_queue_t q);
#endif
