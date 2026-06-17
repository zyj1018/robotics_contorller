/* OSAL FreeRTOS 后端 — 占位 (需链接 FreeRTOS 内核) */
#include "osal.h"
/* 当 FreeRTOS 可用时, 取消注释以下实现 */
#if 0
osal_task_t osal_task_create(const char *name, osal_task_func_t func,
                              void *param, uint32_t stack_size,
                              osal_priority_t priority) {
    TaskHandle_t handle;
    xTaskCreate((TaskFunction_t)func, name, stack_size, param,
                priority, &handle);
    return (osal_task_t)handle;
}
void osal_task_delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
/* ... 更多 FreeRTOS 实现 ... */
#endif
