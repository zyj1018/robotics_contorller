#ifndef CANFD_DRIVER_H
#define CANFD_DRIVER_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    CANFD_STATE_IDLE, CANFD_STATE_ACTIVE,
    CANFD_STATE_ERROR_PASSIVE, CANFD_STATE_BUS_OFF, CANFD_STATE_RECOVERING
} canfd_state_t;

typedef struct { uint32_t id; bool extended; bool fd_format; bool brs;
    uint8_t dlc; uint8_t data[64]; } canfd_frame_t;
typedef struct { uint32_t rx_frames; uint32_t tx_frames; uint32_t bus_off_count;
    uint32_t crc_errors; uint32_t bus_load_permille; } canfd_stats_t;

typedef struct canfd_channel {
    uint8_t channel_id; void *hal_handle; canfd_state_t state;
    canfd_stats_t stats; uint32_t last_rx_timestamp; void *user_data;
} canfd_channel_t;

int canfd_channel_init(canfd_channel_t *ch, uint8_t id);
int canfd_channel_send(canfd_channel_t *ch, const canfd_frame_t *frame);
int canfd_channel_receive(canfd_channel_t *ch, canfd_frame_t *frame, uint32_t timeout_ms);
int canfd_channel_recover_bus_off(canfd_channel_t *ch);
int canfd_channel_get_stats(canfd_channel_t *ch, canfd_stats_t *stats);
#endif
