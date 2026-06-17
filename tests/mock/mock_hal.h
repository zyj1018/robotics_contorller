#ifndef MOCK_HAL_H
#define MOCK_HAL_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Minimal HAL type stubs for PC compilation */
typedef struct { uint8_t dummy; } FDCAN_HandleTypeDef;
typedef struct { uint8_t dummy; } UART_HandleTypeDef;
typedef struct { uint8_t dummy; } DMA_HandleTypeDef;
typedef struct { uint8_t dummy; } SPI_HandleTypeDef;

typedef enum { HAL_OK=0, HAL_ERROR=1, HAL_BUSY=2, HAL_TIMEOUT=3 } HAL_StatusTypeDef;

/* Mock control functions */
void mock_hal_reset_all(void);
uint32_t mock_hal_get_tick_ms(void);
void mock_hal_set_tick_ms(uint32_t ms);
#endif
