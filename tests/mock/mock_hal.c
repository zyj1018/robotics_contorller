#include "mock_hal.h"
static uint32_t mock_tick = 0;
void mock_hal_reset_all(void) { mock_tick = 0; }
uint32_t mock_hal_get_tick_ms(void) { return mock_tick; }
void mock_hal_set_tick_ms(uint32_t ms) { mock_tick = ms; }
