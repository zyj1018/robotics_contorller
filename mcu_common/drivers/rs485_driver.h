#ifndef RS485_DRIVER_H
#define RS485_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

/* ---- RS485 事务状态机 ---- */
typedef enum {
    RS485_IDLE, RS485_PREPARE_TX, RS485_TX, RS485_WAIT_TX_DONE,
    RS485_SWITCH_RX, RS485_WAIT_RESPONSE, RS485_PARSE, RS485_COMPLETE,
    RS485_TIMEOUT, RS485_RETRY, RS485_ERROR_RECOVERY
} rs485_txn_state_t;

/* ---- 统计 ---- */
typedef struct {
    uint32_t tx_frames, rx_frames, timeout_count, crc_errors, retry_count;
} rs485_stats_t;

/* ---- 事务 ---- */
#define RS485_TXN_MAX_DATA 256
typedef struct {
    uint32_t id; uint8_t data[RS485_TXN_MAX_DATA];
    uint32_t len; uint32_t timeout_ms; uint32_t deadline_ms;
} rs485_txn_t;

/* ---- DMA 循环接收缓冲区 ---- */
#define RS485_RX_BUF_SIZE 512
typedef struct {
    uint8_t  buffer[RS485_RX_BUF_SIZE];
    volatile uint16_t head;     /* DMA 写入位置 */
    uint16_t tail;              /* 应用读取位置 */
    uint16_t last_idle_pos;     /* UART IDLE 时的 DMA 位置 */
    volatile bool   idle_flag;  /* IDLE 中断标志 */
} rs485_rx_dma_buf_t;

/* ---- 通道上下文 ---- */
typedef struct rs485_channel {
    uint8_t            channel_id;
    void              *uart_handle;     /* UART_HandleTypeDef* */
    void              *tx_dma;          /* DMA_HandleTypeDef* */
    void              *rx_dma;          /* DMA_HandleTypeDef* */
    void              *dir_port;        /* DE/RE GPIO Port */
    uint16_t           dir_pin;         /* DE/RE GPIO Pin */
    rs485_txn_state_t  txn_state;
    rs485_txn_t        current_txn;
    uint8_t            retry_count;
    rs485_rx_dma_buf_t rx_dma_buf;
    rs485_stats_t      stats;
    void              *user_data;
} rs485_channel_t;

/* ---- API ---- */
int  rs485_channel_init(rs485_channel_t *ch, uint8_t id,
                         void *uart_handle, void *tx_dma, void *rx_dma,
                         void *dir_port, uint16_t dir_pin);
int  rs485_submit_transaction(rs485_channel_t *ch, const rs485_txn_t *txn);
int  rs485_channel_poll(rs485_channel_t *ch, uint32_t now_ms);
void rs485_channel_isr_tx_done(rs485_channel_t *ch);
void rs485_channel_isr_idle(rs485_channel_t *ch);
int  rs485_channel_get_rx_data(rs485_channel_t *ch, uint8_t *buf, uint16_t *len);
void rs485_channel_reset(rs485_channel_t *ch);
#endif
