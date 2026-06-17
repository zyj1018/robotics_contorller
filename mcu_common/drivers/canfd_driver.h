#ifndef CANFD_DRIVER_H
#define CANFD_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

/* ---- 通道状态 ---- */
typedef enum {
    CANFD_STATE_IDLE, CANFD_STATE_ACTIVE,
    CANFD_STATE_ERROR_PASSIVE, CANFD_STATE_BUS_OFF, CANFD_STATE_RECOVERING,
} canfd_state_t;

/* ---- CAN FD 帧 ---- */
#define CANFD_MAX_DATA 64
typedef struct {
    uint32_t id; bool extended; bool fd_format; bool brs;
    uint8_t dlc; uint8_t data[CANFD_MAX_DATA];
} canfd_frame_t;

/* ---- 统计 ---- */
typedef struct {
    uint32_t rx_frames, tx_frames, rx_overrun, tx_fifo_overflow;
    uint32_t crc_errors, form_errors, ack_errors;
    uint32_t bus_off_count, last_error_code, bus_load_permille;
} canfd_stats_t;

/* ---- 环形缓冲区 ---- */
#define CANFD_RING_SIZE 64
typedef struct {
    canfd_frame_t frames[CANFD_RING_SIZE];
    volatile uint8_t head, tail, count;
} canfd_ring_t;

/* ---- 通道上下文 ---- */
typedef struct canfd_channel {
    uint8_t channel_id; void *fdcan_handle;
    canfd_state_t state; canfd_ring_t rx_ring;
    canfd_stats_t stats; uint32_t last_rx_timestamp; void *user_data;
} canfd_channel_t;

/* ---- API ---- */
int  canfd_channel_init(canfd_channel_t *ch, uint8_t id, void *hal_handle);
int  canfd_channel_send(canfd_channel_t *ch, const canfd_frame_t *frame);
int  canfd_channel_receive(canfd_channel_t *ch, canfd_frame_t *frame, uint32_t timeout_ms);
void canfd_channel_isr_rx(canfd_channel_t *ch);
void canfd_channel_isr_error(canfd_channel_t *ch, uint32_t error_code);
int  canfd_channel_recover_bus_off(canfd_channel_t *ch);
int  canfd_channel_get_stats(canfd_channel_t *ch, canfd_stats_t *stats);
#endif
