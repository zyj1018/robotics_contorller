#ifndef RS485_DRIVER_H
#define RS485_DRIVER_H
#include <stdint.h>

typedef enum { RS485_IDLE, RS485_PREPARE_TX, RS485_TX, RS485_WAIT_TX_DONE,
    RS485_SWITCH_RX, RS485_WAIT_RESPONSE, RS485_PARSE, RS485_COMPLETE,
    RS485_TIMEOUT, RS485_RETRY, RS485_ERROR_RECOVERY } rs485_txn_state_t;
typedef struct { uint32_t tx_frames; uint32_t rx_frames; uint32_t timeout_count;
    uint32_t crc_errors; uint32_t retry_count; } rs485_stats_t;

typedef struct { uint32_t id; uint8_t *data; uint32_t len; uint32_t timeout_ms; } rs485_txn_t;

typedef struct rs485_channel {
    uint8_t channel_id; void *uart_handle; void *tx_dma; void *rx_dma;
    rs485_txn_state_t txn_state; uint8_t retry_count;
    uint32_t txn_deadline; rs485_stats_t stats; void *user_data;
} rs485_channel_t;

int rs485_channel_init(rs485_channel_t *ch, uint8_t id);
int rs485_submit_transaction(rs485_channel_t *ch, const rs485_txn_t *txn);
int rs485_cancel_transaction(rs485_channel_t *ch, uint32_t txn_id);
int rs485_channel_poll(rs485_channel_t *ch);
#endif
