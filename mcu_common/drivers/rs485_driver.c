#include "rs485_driver.h"
#include <string.h>
#ifdef DRIVER_BACKEND_HAL
#ifdef DRIVER_BACKEND_HAL
#include "stm32g4xx_hal.h"
#endif
#endif

#define RS485_MAX_RETRIES 3

/* ---- 方向控制 ---- */
static void rs485_set_tx(rs485_channel_t *ch) { (void)ch;
#ifdef DRIVER_BACKEND_HAL
    HAL_GPIO_WritePin((GPIO_TypeDef*)ch->dir_port, ch->dir_pin, GPIO_PIN_SET);
#endif
}
static void rs485_set_rx(rs485_channel_t *ch) { (void)ch;
#ifdef DRIVER_BACKEND_HAL
    HAL_GPIO_WritePin((GPIO_TypeDef*)ch->dir_port, ch->dir_pin, GPIO_PIN_RESET);
#endif
}

/* ---- 初始化 ---- */
int rs485_channel_init(rs485_channel_t *ch, uint8_t id,
                        void *uart_handle, void *tx_dma, void *rx_dma,
                        void *dir_port, uint16_t dir_pin) {
    if (!ch) return -1;
    memset(ch, 0, sizeof(*ch));
    ch->channel_id = id;
    ch->uart_handle = uart_handle;
    ch->tx_dma = tx_dma;
    ch->rx_dma = rx_dma;
    ch->dir_port = dir_port;
    ch->dir_pin = dir_pin;
    ch->txn_state = RS485_IDLE;
    rs485_set_rx(ch);

    /* 启动 DMA 循环接收 */
#ifdef DRIVER_BACKEND_HAL
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart_handle;
    HAL_UARTEx_ReceiveToIdle_DMA(huart, ch->rx_dma_buf.buffer, RS485_RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT((DMA_HandleTypeDef*)rx_dma, DMA_IT_HT); /* 禁用半传输中断 */
#endif
    return 0;
}

/* ---- 提交事务 ---- */
int rs485_submit_transaction(rs485_channel_t *ch, const rs485_txn_t *txn) {
    if (!ch || !txn || ch->txn_state != RS485_IDLE) return -1;
    memcpy(&ch->current_txn, txn, sizeof(rs485_txn_t));
    ch->retry_count = 0;
    ch->txn_state = RS485_PREPARE_TX;
    return 0;
}

/* ---- 事务状态机轮询 (1ms tick) ---- */
int rs485_channel_poll(rs485_channel_t *ch, uint32_t now_ms) {
    if (!ch) return -1;

    switch (ch->txn_state) {
    case RS485_IDLE:
    case RS485_COMPLETE:
    case RS485_ERROR_RECOVERY:
        return 0;

    case RS485_PREPARE_TX:
        ch->current_txn.deadline_ms = now_ms + ch->current_txn.timeout_ms;
        rs485_set_tx(ch);
        ch->txn_state = RS485_TX;
#ifdef DRIVER_BACKEND_HAL
        {
            UART_HandleTypeDef *huart = (UART_HandleTypeDef *)ch->uart_handle;
            HAL_UART_Transmit_DMA(huart, ch->current_txn.data, ch->current_txn.len);
        }
#endif
        break;

    case RS485_WAIT_TX_DONE:
        /* DMA TC 中断会触发 → RS485_SWITCH_RX */
        break;

    case RS485_WAIT_RESPONSE:
        if (ch->rx_dma_buf.idle_flag) {
            ch->rx_dma_buf.idle_flag = false;
            ch->txn_state = RS485_PARSE;
        } else if (now_ms > ch->current_txn.deadline_ms) {
            ch->txn_state = RS485_TIMEOUT;
        }
        break;

    case RS485_PARSE:
        ch->stats.rx_frames++;
        ch->txn_state = RS485_COMPLETE;
        break;

    case RS485_TIMEOUT:
        ch->stats.timeout_count++;
        if (++ch->retry_count < RS485_MAX_RETRIES) {
            ch->txn_state = RS485_RETRY;
        } else {
            ch->txn_state = RS485_ERROR_RECOVERY;
        }
        break;

    case RS485_RETRY:
        ch->txn_state = RS485_PREPARE_TX;
        break;

    default:
        break;
    }
    return 0;
}

/* ---- ISR: DMA 发送完成 ---- */
void rs485_channel_isr_tx_done(rs485_channel_t *ch) {
    if (!ch || ch->txn_state != RS485_TX) return;
    ch->stats.tx_frames++;
    rs485_set_rx(ch);
    ch->txn_state = RS485_WAIT_RESPONSE;
}

/* ---- ISR: UART IDLE (帧接收完成) ---- */
void rs485_channel_isr_idle(rs485_channel_t *ch) {
    if (!ch) return;
#ifdef DRIVER_BACKEND_HAL
    ch->rx_dma_buf.last_idle_pos = RS485_RX_BUF_SIZE -
        __HAL_DMA_GET_COUNTER((DMA_HandleTypeDef*)ch->rx_dma);
    ch->rx_dma_buf.idle_flag = true;
#endif
}

/* ---- 获取接收数据 ---- */
int rs485_channel_get_rx_data(rs485_channel_t *ch, uint8_t *buf, uint16_t *len) {
    if (!ch || !buf || !len) return -1;
    uint16_t avail = (ch->rx_dma_buf.last_idle_pos >= ch->rx_dma_buf.tail)
        ? (ch->rx_dma_buf.last_idle_pos - ch->rx_dma_buf.tail)
        : (RS485_RX_BUF_SIZE - ch->rx_dma_buf.tail + ch->rx_dma_buf.last_idle_pos);
    if (avail > *len) avail = *len;
    if (avail == 0) { *len = 0; return 0; }
    /* 简化: 线性拷贝 (实际应处理环形回绕) */
    memcpy(buf, ch->rx_dma_buf.buffer + ch->rx_dma_buf.tail, avail);
    ch->rx_dma_buf.tail = (uint16_t)((ch->rx_dma_buf.tail + avail) % RS485_RX_BUF_SIZE);
    *len = avail;
    return 0;
}

/* ---- 通道复位 ---- */
void rs485_channel_reset(rs485_channel_t *ch) {
    if (!ch) return;
    ch->txn_state = RS485_IDLE;
    ch->retry_count = 0;
    rs485_set_rx(ch);
    ch->rx_dma_buf.idle_flag = false;
#ifdef DRIVER_BACKEND_HAL
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)ch->uart_handle;
    HAL_UART_AbortReceive(huart);
    HAL_UARTEx_ReceiveToIdle_DMA(huart, ch->rx_dma_buf.buffer, RS485_RX_BUF_SIZE);
#endif
}
