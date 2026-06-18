#include "link_manager.h"
#include <string.h>

/* 链路优先级: SPI(主) > USB(维护) > USART(救援) */
#define SPI_ERR_THRESHOLD  10
#define HEARTBEAT_TIMEO_MS 50

void link_manager_init(link_manager_t *mgr) {
    memset(mgr, 0, sizeof(*mgr));
    mgr->links[LINK_SPI].type   = LINK_SPI;
    mgr->links[LINK_USB].type   = LINK_USB;
    mgr->links[LINK_USART].type = LINK_USART;
    mgr->links[LINK_SPI].state  = LINK_DOWN;
    mgr->links[LINK_USB].state  = LINK_DOWN;
    mgr->links[LINK_USART].state= LINK_DOWN;
    mgr->active_link = LINK_NONE;
}

/* ---- 核心决策: 每条入站消息都要经过此函数 ---- */
link_decision_t link_manager_evaluate(link_manager_t *mgr, link_type_t src,
                                       uint8_t msg_type, uint32_t session_id,
                                       uint32_t seq, uint32_t timestamp,
                                       uint32_t ttl_ms, uint32_t now_ms) {
    if (!mgr) return LINK_DECISION_REJECT;

    /* 1. 链路状态检查 */
    if (mgr->links[src].state == LINK_DOWN)
        return LINK_DECISION_IGNORE;

    /* 2. USART 救援模式: 仅允许有限命令 */
    if (src == LINK_USART && mgr->links[LINK_USART].rescue_mode) {
        switch (msg_type) {
            case MSG_FIRMWARE_UPGRADE:
            case MSG_BOOTLOADER_CTRL:
            case MSG_VERSION_QUERY:
            case MSG_CAPABILITY_QUERY:
                break; /* 允许 */
            default:
                return LINK_DECISION_REJECT;
        }
    }

    /* 3. USB 维护模式: 禁止控制命令 */
    if (src == LINK_USB) {
        if (msg_type == MSG_REALTIME_CONTROL)
            return LINK_DECISION_REJECT;
    }

    /* 4. 会话检查 */
    if (mgr->session_active && session_id != 0 &&
        msg_type != MSG_SESSION_ESTABLISH) {
        if (session_id != mgr->active_session_id)
            return LINK_DECISION_REJECT; /* 旧会话或无效会话 */
    }

    /* 5. 控制命令额外校验 (仅 SPI 可下发控制) */
    if (msg_type == MSG_REALTIME_CONTROL) {
        if (src != LINK_SPI)
            return LINK_DECISION_REJECT;
        if (!mgr->control_granted)
            return LINK_DECISION_REJECT;

        /* 序列号单调递增 */
        if (seq <= mgr->last_accepted_seq)
            return LINK_DECISION_REJECT;
        /* TTL 检查 */
        if (ttl_ms > 0 && (now_ms - timestamp) > ttl_ms)
            return LINK_DECISION_REJECT;

        mgr->last_accepted_seq = seq;
        mgr->last_accepted_ts  = timestamp;
    }

    /* 6. 会话建立 */
    if (msg_type == MSG_SESSION_ESTABLISH) {
        link_manager_session_start(mgr, session_id);
    }

    /* 7. 心跳: 更新链路活跃时间 */
    if (msg_type == MSG_HEARTBEAT) {
        mgr->links[src].last_rx_timestamp_ms = now_ms;
        mgr->links[src].consecutive_errors = 0;
    }

    return LINK_DECISION_ACCEPT;
}

/* ---- 链路事件 ---- */
void link_manager_link_up(link_manager_t *mgr, link_type_t link) {
    if (!mgr || link >= 4) return;
    mgr->links[link].state = LINK_UP;
    mgr->links[link].consecutive_errors = 0;

    /* 自动选择最高优先级链路 */
    if (link == LINK_SPI) {
        mgr->active_link = LINK_SPI;
        mgr->control_granted = true;
    } else if (mgr->active_link == LINK_NONE) {
        mgr->active_link = link;
    }
}

void link_manager_link_down(link_manager_t *mgr, link_type_t link) {
    if (!mgr || link >= 4) return;
    mgr->links[link].state = LINK_DOWN;

    /* SPI 断开: 撤销控制权, 进入安全状态 */
    if (link == LINK_SPI) {
        mgr->control_granted = false;
        mgr->session_active = false;
    }
    /* 回退到次高优先级 */
    if (mgr->active_link == link) {
        if (mgr->links[LINK_USB].state == LINK_UP)
            mgr->active_link = LINK_USB;
        else if (mgr->links[LINK_USART].state == LINK_UP)
            mgr->active_link = LINK_USART;
        else
            mgr->active_link = LINK_NONE;
    }
}

void link_manager_session_start(link_manager_t *mgr, uint32_t session_id) {
    if (!mgr) return;
    mgr->active_session_id = session_id;
    mgr->last_accepted_seq = 0;
    mgr->last_accepted_ts  = 0;
    mgr->session_active = true;
}

void link_manager_session_end(link_manager_t *mgr) {
    if (!mgr) return;
    mgr->session_active = false;
    mgr->control_granted = false;
}

bool link_manager_can_control(link_manager_t *mgr, link_type_t src) {
    return mgr && src == LINK_SPI && mgr->control_granted;
}

link_type_t link_manager_active_link(const link_manager_t *mgr) {
    return mgr ? mgr->active_link : LINK_NONE;
}

void link_manager_record_rx(link_manager_t *mgr, link_type_t link, uint32_t bytes) {
    if (mgr && link < 4) mgr->links[link].rx_bytes += bytes;
}
void link_manager_record_tx(link_manager_t *mgr, link_type_t link, uint32_t bytes) {
    if (mgr && link < 4) mgr->links[link].tx_bytes += bytes;
}
