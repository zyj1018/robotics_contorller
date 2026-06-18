#ifndef LINK_MANAGER_H
#define LINK_MANAGER_H
#include <stdint.h>
#include <stdbool.h>
#include "protocol_types.h"

/* ---- 链路类型 ---- */
typedef enum { LINK_NONE=0, LINK_SPI, LINK_USB, LINK_USART } link_type_t;
typedef enum { LINK_UP, LINK_DOWN, LINK_DEGRADED } link_state_t;

/* ---- 链路入口 ---- */
typedef struct {
    link_type_t  type;
    link_state_t state;
    uint32_t     error_count;
    uint32_t     consecutive_errors;
    uint32_t     last_rx_timestamp_ms;
    uint32_t     rx_bytes;
    uint32_t     tx_bytes;
    bool         debug_permitted;     /* USB 调试权限 */
    bool         rescue_mode;         /* USART 救援模式 */
} link_entry_t;

/* ---- Link Manager ---- */
typedef struct {
    link_entry_t links[4];            /* index by link_type_t */
    link_type_t  active_link;         /* 当前主链路 */
    uint32_t     active_session_id;   /* 当前有效会话 */
    uint32_t     last_accepted_seq;   /* 最后接受的序列号 */
    uint32_t     last_accepted_ts;    /* 最后接受的时间戳 */
    bool         session_active;
    bool         control_granted;     /* SPI 是否授予控制权 */
} link_manager_t;

/* ---- 链路决策 ---- */
typedef enum {
    LINK_DECISION_ACCEPT,
    LINK_DECISION_REJECT,
    LINK_DECISION_ENTER_SAFE,
    LINK_DECISION_RETRY,
    LINK_DECISION_SESSION_RESET,
    LINK_DECISION_IGNORE,
} link_decision_t;

/* ---- API ---- */
void link_manager_init(link_manager_t *mgr);
link_decision_t link_manager_evaluate(link_manager_t *mgr, link_type_t src,
                                       uint8_t msg_type, uint32_t session_id,
                                       uint32_t seq, uint32_t timestamp,
                                       uint32_t ttl_ms, uint32_t now_ms);
void link_manager_link_up(link_manager_t *mgr, link_type_t link);
void link_manager_link_down(link_manager_t *mgr, link_type_t link);
void link_manager_session_start(link_manager_t *mgr, uint32_t session_id);
void link_manager_session_end(link_manager_t *mgr);
bool link_manager_can_control(link_manager_t *mgr, link_type_t src);
link_type_t link_manager_active_link(const link_manager_t *mgr);
void link_manager_record_rx(link_manager_t *mgr, link_type_t link, uint32_t bytes);
void link_manager_record_tx(link_manager_t *mgr, link_type_t link, uint32_t bytes);
#endif
