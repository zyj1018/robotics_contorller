/* 协议会话管理实现 */
#include "protocol_session.h"
#include "protocol_types.h"
#include <stdlib.h>
#include <string.h>

/* 简单的伪随机数 (MCU 环境可能没有 rand()) */
static uint32_t _simple_random_seed = 0xDEADBEEF;
static uint32_t _simple_random(void) {
    _simple_random_seed = _simple_random_seed * 1103515245U + 12345U;
    return _simple_random_seed;
}

void protocol_session_init(ProtocolSession *session, uint32_t heartbeat_timeout_ms) {
    memset(session, 0, sizeof(*session));
    session->heartbeat_timeout_ms = heartbeat_timeout_ms;
}

uint32_t protocol_session_generate_id(void) {
    return _simple_random() ^ (_simple_random() << 13);
}

void protocol_session_start(ProtocolSession *session, uint32_t new_session_id,
                             uint8_t peer_node, uint8_t peer_version) {
    session->session_id = new_session_id;
    session->peer_node_id = peer_node;
    session->peer_proto_version = peer_version;
    session->last_accepted_seq = 0;
    session->last_accepted_timestamp = 0;
    session->tx_sequence = 0;
    session->session_active = true;
}

void protocol_session_end(ProtocolSession *session) {
    session->session_active = false;
    session->session_id = 0;
}

uint32_t protocol_session_next_tx_seq(ProtocolSession *session) {
    return ++session->tx_sequence; /* 从 1 开始 */
}

bool protocol_session_is_peer_alive(const ProtocolSession *session, uint32_t now_ms) {
    if (!session->session_active) return false;
    uint32_t elapsed = now_ms - session->last_heartbeat_rx;
    return elapsed < session->heartbeat_timeout_ms;
}

void protocol_session_heartbeat_rx(ProtocolSession *session, uint32_t now_ms,
                                    uint8_t safety_state) {
    session->last_heartbeat_rx = now_ms;
    session->peer_safety_state = safety_state;
}

int protocol_session_validate_inbound(ProtocolSession *session,
                                       uint32_t seq, uint32_t timestamp,
                                       uint32_t ttl_ms, uint32_t session_id,
                                       uint32_t now_ms) {
    if (!session->session_active)
        return PROTO_ERR_SESSION_INVALID;

    if (session_id != session->session_id)
        return PROTO_ERR_SESSION_INVALID;

    if (seq <= session->last_accepted_seq)
        return PROTO_ERR_SEQ_OUTDATED;

    if (timestamp < session->last_accepted_timestamp)
        return PROTO_ERR_SEQ_OUTDATED;

    if (ttl_ms > 0) {
        uint32_t elapsed = now_ms - timestamp;
        if (elapsed > ttl_ms)
            return PROTO_ERR_TTL_EXPIRED;
    }

    /* 更新状态 */
    session->last_accepted_seq = seq;
    session->last_accepted_timestamp = timestamp;

    return PROTO_OK;
}
