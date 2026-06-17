/* Minimal FreeRTOS stubs for build verification */
#ifndef FREERTOS_STUB_H
#define FREERTOS_STUB_H
#include <stdint.h>
#include <stdbool.h>
typedef void * TaskHandle_t;
typedef void * QueueHandle_t;
typedef void * SemaphoreHandle_t;
typedef void * EventGroupHandle_t;
typedef void * TimerHandle_t;
typedef uint32_t TickType_t;
typedef void (*TaskFunction_t)(void *);
#define pdMS_TO_TICKS(ms) (ms)
#define pdFALSE 0
#define pdTRUE 1
#define pdPASS 1
#define pdFAIL 0
#endif
