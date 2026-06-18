#include "canfd_driver.h"
#include <string.h>
#ifdef DRIVER_BACKEND_HAL
#ifdef DRIVER_BACKEND_HAL
#include "stm32g4xx_hal.h"
#endif
#endif

/* ---- 环形缓冲操作 ---- */
static inline bool ring_push(canfd_ring_t *r, const canfd_frame_t *f) {
    if (r->count >= CANFD_RING_SIZE) { r->count = CANFD_RING_SIZE; return false; }
    memcpy(&r->frames[r->head], f, sizeof(canfd_frame_t));
    r->head = (uint8_t)((r->head + 1) % CANFD_RING_SIZE);
    r->count++;
    return true;
}
static inline bool ring_pop(canfd_ring_t *r, canfd_frame_t *f) {
    if (r->count == 0) return false;
    memcpy(f, &r->frames[r->tail], sizeof(canfd_frame_t));
    r->tail = (uint8_t)((r->tail + 1) % CANFD_RING_SIZE);
    r->count--;
    return true;
}

/* ---- 初始化 ---- */
int canfd_channel_init(canfd_channel_t *ch, uint8_t id, void *hal_handle) {
    if (!ch) return -1;
    memset(ch, 0, sizeof(*ch));
    ch->channel_id = id;
    ch->fdcan_handle = hal_handle;
    ch->state = CANFD_STATE_ACTIVE;
    return 0;
}

/* ---- 发送 ---- */
int canfd_channel_send(canfd_channel_t *ch, const canfd_frame_t *frame) {
    if (!ch || !frame) return -1;
#ifdef DRIVER_BACKEND_HAL
    FDCAN_HandleTypeDef *h = (FDCAN_HandleTypeDef *)ch->fdcan_handle;
    if (!h || ch->state == CANFD_STATE_BUS_OFF) return -1;

    FDCAN_TxHeaderTypeDef txh = {0};
    txh.Identifier = frame->id;
    txh.IdType = frame->extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    txh.TxFrameType = FDCAN_DATA_FRAME;
    txh.DataLength = canfd_bytes_to_dlc(frame->dlc);
    txh.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txh.BitRateSwitch = frame->brs ? FDCAN_BRS_ON : FDCAN_BRS_OFF;
    txh.FDFormat = frame->fd_format ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;
    txh.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txh.MessageMarker = 0;

    if (HAL_FDCAN_AddMessageToTxFifoQ(h, &txh, frame->data) != HAL_OK)
        return -1;
    ch->stats.tx_frames++;
    return 0;
#else
    (void)ch; (void)frame;
    (void)ch; (void)frame; return -1;
#endif
}

/* ---- 接收 (从环形缓冲区取帧) ---- */
int canfd_channel_receive(canfd_channel_t *ch, canfd_frame_t *frame, uint32_t timeout_ms) {
    if (!ch || !frame) return -1;
    (void)timeout_ms;
    if (ring_pop(&ch->rx_ring, frame)) return 0;
    return -1;
}

/* ---- ISR: CAN RX FIFO 回调 ---- */
void canfd_channel_isr_rx(canfd_channel_t *ch) {
    if (!ch) return;
#ifdef DRIVER_BACKEND_HAL
    FDCAN_HandleTypeDef *h = (FDCAN_HandleTypeDef *)ch->fdcan_handle;
    FDCAN_RxHeaderTypeDef rxh;
    canfd_frame_t frame;

    while (HAL_FDCAN_GetRxMessage(h, FDCAN_RX_FIFO0, &rxh, frame.data) == HAL_OK) {
        frame.id = rxh.Identifier;
        frame.extended = (rxh.IdType == FDCAN_EXTENDED_ID);
        frame.fd_format = (rxh.FDFormat == FDCAN_FD_CAN);
        frame.brs = (rxh.BitRateSwitch == FDCAN_BRS_ON);
        frame.dlc = canfd_dlc_to_bytes(rxh.DataLength);
        ring_push(&ch->rx_ring, &frame);
        ch->stats.rx_frames++;
    }
#endif
}

/* ---- ISR: 错误处理 ---- */
void canfd_channel_isr_error(canfd_channel_t *ch, uint32_t error_code) {
    if (!ch) return;
    ch->stats.last_error_code = error_code;
#ifdef DRIVER_BACKEND_HAL
    FDCAN_HandleTypeDef *h = (FDCAN_HandleTypeDef *)ch->fdcan_handle;
    if (h->ErrorCode & FDCAN_ERRSR_BOFF) {
        ch->state = CANFD_STATE_BUS_OFF;
        ch->stats.bus_off_count++;
    } else if (h->ErrorCode & FDCAN_ERRSR_EPASS) {
        ch->state = CANFD_STATE_ERROR_PASSIVE;
    }
#endif
}

/* ---- Bus Off 恢复 ---- */
int canfd_channel_recover_bus_off(canfd_channel_t *ch) {
    if (!ch || ch->state != CANFD_STATE_BUS_OFF) return -1;
    ch->state = CANFD_STATE_RECOVERING;
#ifdef DRIVER_BACKEND_HAL
    FDCAN_HandleTypeDef *h = (FDCAN_HandleTypeDef *)ch->fdcan_handle;
    HAL_FDCAN_Stop(h);
    HAL_FDCAN_Start(h);
#endif
    ch->state = CANFD_STATE_ACTIVE;
    return 0;
}


/* ---- DLC 转换 (CAN FD: 0-8/12/16/20/24/32/48/64 bytes) ---- */
uint8_t canfd_bytes_to_dlc(uint8_t len) {
    if (len <= 8)  return len;
    if (len <= 12) return 9;
    if (len <= 16) return 10;
    if (len <= 20) return 11;
    if (len <= 24) return 12;
    if (len <= 32) return 13;
    if (len <= 48) return 14;
    return 15; /* 64 */
}
uint8_t canfd_dlc_to_bytes(uint8_t dlc) {
    static const uint8_t map[] = {0,1,2,3,4,5,6,7,8,12,16,20,24,32,48,64};
    return (dlc < 16) ? map[dlc] : 64;
}

int canfd_channel_get_stats(canfd_channel_t *ch, canfd_stats_t *stats) {
    if (!ch || !stats) return -1;
    memcpy(stats, &ch->stats, sizeof(canfd_stats_t));
    return 0;
}
