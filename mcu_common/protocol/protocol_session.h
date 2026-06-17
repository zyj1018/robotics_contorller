/* 协议会话管理 */
#ifndef PROTOCOL_SESSION_H
#define PROTOCOL_SESSION_H
#include <stdint.h>
#include <stdbool.h>

/* 单个 MCU 端点会话状态 */
typedef struct {
    uint32_t session_id;            /* 当前活动会话 ID */
    uint32_t last_accepted_seq;     /* 最后接受的序列号 */
    uint32_t last_accepted_timestamp; /* 最后接受的时间戳 */
    uint32_t tx_sequence;           /* 本端发送序列号 */
    uint32_t session_start_time;    /* 会话开始时间戳 */
    uint8_t  peer_proto_version;    /* 对端协议版本 */
    uint8_t  peer_node_id;          /* 对端节点 ID */
    bool     session_active;        /* 会话是否活跃 */
    uint32_t heartbeat_timeout_ms;  /* 心跳超时 (ms) */
    uint32_t last_heartbeat_rx;     /* 上次收到心跳时间 */
    uint8_t  peer_safety_state;     /* 对端安全状态 */
} ProtocolSession;

/* 初始化会话 */
void protocol_session_init(ProtocolSession *session, uint32_t heartbeat_timeout_ms);

/* 生成新会话 ID (随机) */
uint32_t protocol_session_generate_id(void);

/* 开始新会话 */
void protocol_session_start(ProtocolSession *session, uint32_t new_session_id,
                             uint8_t peer_node, uint8_t peer_version);

/* 结束会话 */
void protocol_session_end(ProtocolSession *session);

/* 获取下一个发送序列号 (原子递增) */
uint32_t protocol_session_next_tx_seq(ProtocolSession *session);

/* 检查对端是否存活 (心跳未超时) */
bool protocol_session_is_peer_alive(const ProtocolSession *session, uint32_t now_ms);

/* 处理收到的心跳 */
void protocol_session_heartbeat_rx(ProtocolSession *session, uint32_t now_ms,
                                    uint8_t safety_state);

/* 验证入站命令 (序列号+时间戳+有效期+会话) */
int protocol_session_validate_inbound(ProtocolSession *session,
                                       uint32_t seq, uint32_t timestamp,
                                       uint32_t ttl_ms, uint32_t session_id,
                                       uint32_t now_ms);

#endif /* PROTOCOL_SESSION_H */
