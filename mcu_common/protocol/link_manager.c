#include "link_manager.h"
#include <string.h>

#define SPI_ERR_THRESHOLD  10
#define HEARTBEAT_TIMEO_MS 50

/* ISSUE-003: 共享链路类型验证 */
static bool link_type_is_valid(link_type_t link) {
    return link >= LINK_SPI && link <= LINK_USART;
}

void link_manager_init(link_manager_t *mgr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(*mgr));
    mgr->links[LINK_SPI].type   = LINK_SPI;
    mgr->links[LINK_USB].type   = LINK_USB;
    mgr->links[LINK_USART].type = LINK_USART;
    mgr->links[LINK_SPI].state  = LINK_DOWN;
    mgr->links[LINK_USB].state  = LINK_DOWN;
    mgr->links[LINK_USART].state= LINK_DOWN;
    mgr->active_link = LINK_NONE;
}

link_decision_t link_manager_evaluate(link_manager_t *mgr, link_type_t src,
                                       uint8_t msg_type, uint32_t session_id,
                                       uint32_t seq, uint32_t timestamp,
                                       uint32_t ttl_ms, uint32_t now_ms) {
    if (!mgr || !link_type_is_valid(src)) return LINK_DECISION_REJECT;

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
                break;
            default:
                return LINK_DECISION_REJECT;
        }
    }

    /* 3. USB 维护模式: 禁止控制命令 */
    if (src == LINK_USB && msg_type == MSG_REALTIME_CONTROL)
        return LINK_DECISION_REJECT;

    /* 4. 会话建立: 仅 SPI 可以建立会话, 拒绝 session_id==0 */
    if (msg_type == MSG_SESSION_ESTABLISH) {
        if (src != LINK_SPI || session_id == 0)
            return LINK_DECISION_REJECT;
        link_manager_session_start(mgr, session_id);
        return LINK_DECISION_ACCEPT;
    }

    /* 5. 会话终止 */
    if (msg_type == MSG_SESSION_TERMINATE) {
        if (src != LINK_SPI || session_id != mgr->active_session_id)
            return LINK_DECISION_REJECT;
        link_manager_session_end(mgr);
        return LINK_DECISION_ACCEPT;
    }

    /* 6. 实时控制命令: 必须通过 SPI + 活跃会话 + 控制权 */
    if (msg_type == MSG_REALTIME_CONTROL) {
        if (src != LINK_SPI)
            return LINK_DECISION_REJECT;
        if (!mgr->session_active)
            return LINK_DECISION_REJECT;
        if (session_id == 0 || session_id != mgr->active_session_id)
            return LINK_DECISION_REJECT;
        if (!mgr->control_granted)
            return LINK_DECISION_REJECT;
        if (seq <= mgr->last_accepted_seq)
            return LINK_DECISION_REJECT;
        if (ttl_ms > 0 && (now_ms - timestamp) > ttl_ms)
            return LINK_DECISION_REJECT;

        mgr->last_accepted_seq = seq;
        mgr->last_accepted_ts  = timestamp;
    }

    /* 7. 心跳: 更新链路活跃时间 */
    if (msg_type == MSG_HEARTBEAT) {
        mgr->links[src].last_rx_timestamp_ms = now_ms;
        mgr->links[src].consecutive_errors = 0;
    }

    return LINK_DECISION_ACCEPT;
}

void link_manager_link_up(link_manager_t *mgr, link_type_t link) {
    if (!mgr || !link_type_is_valid(link)) return;
    mgr->links[link].state = LINK_UP;
    mgr->links[link].consecutive_errors = 0;

    if (link == LINK_SPI) {
        mgr->active_link = LINK_SPI;
        /* 注意: control_granted 只在 session_start 后生效 */
    } else if (mgr->active_link == LINK_NONE) {
        mgr->active_link = link;
    }
}

void link_manager_link_down(link_manager_t *mgr, link_type_t link) {
    if (!mgr || !link_type_is_valid(link)) return;
    mgr->links[link].state = LINK_DOWN;

    if (link == LINK_SPI) {
        mgr->control_granted = false;
        mgr->session_active = false;
    }
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
    if (!mgr || session_id == 0) return;
    mgr->active_session_id = session_id;
    mgr->last_accepted_seq = 0;
    mgr->last_accepted_ts  = 0;
    mgr->session_active = true;
    mgr->control_granted = true; /* ISSUE-002: 会话建立时授予控制权 */
}

void link_manager_session_end(link_manager_t *mgr) {
    if (!mgr) return;
    mgr->session_active = false;
    mgr->control_granted = false;
}

bool link_manager_can_control(link_manager_t *mgr, link_type_t src) {
    return mgr && src == LINK_SPI && mgr->control_granted && mgr->session_active;
}

link_type_t link_manager_active_link(const link_manager_t *mgr) {
    return mgr ? mgr->active_link : LINK_NONE;
}

void link_manager_record_rx(link_manager_t *mgr, link_type_t link, uint32_t bytes) {
    if (mgr && link_type_is_valid(link)) mgr->links[link].rx_bytes += bytes;
}
void link_manager_record_tx(link_manager_t *mgr, link_type_t link, uint32_t bytes) {
    if (mgr && link_type_is_valid(link)) mgr->links[link].tx_bytes += bytes;
}
