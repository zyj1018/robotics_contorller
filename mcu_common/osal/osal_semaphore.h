#ifndef OSAL_SEMAPHORE_H
#define OSAL_SEMAPHORE_H
#include "osal_types.h"
osal_sem_t osal_sem_create_binary(void);
osal_sem_t osal_sem_create_counting(uint32_t max, uint32_t initial);
int osal_sem_take(osal_sem_t sem, uint32_t timeout_ms);
int osal_sem_give(osal_sem_t sem);
void osal_sem_delete(osal_sem_t sem);
osal_sem_t osal_mutex_create(void);
int osal_mutex_lock(osal_sem_t mutex);
int osal_mutex_unlock(osal_sem_t mutex);
#endif
