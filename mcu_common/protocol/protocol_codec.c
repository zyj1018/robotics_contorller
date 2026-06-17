/* 协议编解码实现 */
#include "protocol_codec.h"
#include "protocol_crc.h"
#include <string.h>

/* ---- Header 序列化 ---- */
int protocol_header_serialize(const ProtocolHeader *header, uint8_t *buffer, size_t buf_size) {
    if (!header || !buffer || buf_size < PROTOCOL_HEADER_LENGTH)
        return PROTO_ERR_BUFFER_FULL;

    uint8_t *p = buffer;
    p[0]  = (uint8_t)(header->sync);            p[1]  = (uint8_t)(header->sync >> 8);
    p[2]  = header->proto_version;              p[3]  = header->header_length;
    p[4]  = header->msg_type;                   p[5]  = header->msg_subtype;
    p[6]  = header->src_node_id;                p[7]  = header->dst_node_id;
    p[8]  = header->mcu_role;

    /* session_id (LE) */
    p[9]  = (uint8_t)(header->session_id);       p[10] = (uint8_t)(header->session_id >> 8);
    p[11] = (uint8_t)(header->session_id >> 16); p[12] = (uint8_t)(header->session_id >> 24);

    /* sequence_number (LE) */
    p[13] = (uint8_t)(header->sequence_number);       p[14] = (uint8_t)(header->sequence_number >> 8);
    p[15] = (uint8_t)(header->sequence_number >> 16); p[16] = (uint8_t)(header->sequence_number >> 24);

    /* timestamp_ms (LE) */
    p[17] = (uint8_t)(header->timestamp_ms);       p[18] = (uint8_t)(header->timestamp_ms >> 8);
    p[19] = (uint8_t)(header->timestamp_ms >> 16); p[20] = (uint8_t)(header->timestamp_ms >> 24);

    /* cmd_ttl_ms (LE) */
    p[21] = (uint8_t)(header->cmd_ttl_ms);         p[22] = (uint8_t)(header->cmd_ttl_ms >> 8);

    /* payload_length (LE) */
    p[23] = (uint8_t)(header->payload_length);     p[24] = (uint8_t)(header->payload_length >> 8);

    /* flags (LE) */
    p[25] = (uint8_t)(header->flags);              p[26] = (uint8_t)(header->flags >> 8);

    /* fragment */
    p[27] = (uint8_t)(header->fragment_index);     p[28] = (uint8_t)(header->fragment_index >> 8);
    p[29] = header->fragment_total;

    /* CRC8 (字节 0-29 计算, 填入 30) */
    crc8_fill_header(buffer);

    return PROTO_OK;
}

/* ---- 完整帧序列化 ---- */
int protocol_frame_serialize(const ProtocolHeader *header,
                              const uint8_t *payload, uint16_t payload_len,
                              uint8_t *buffer, size_t buf_size, size_t *out_len) {
    if (!header || !buffer) return PROTO_ERR_PARAM_ERR;

    size_t total = PROTOCOL_HEADER_LENGTH + payload_len + 2; /* +2 CRC16 */
    if (buf_size < total) return PROTO_ERR_BUFFER_FULL;

    /* 序列化 Header */
    ProtocolHeader h = *header;
    h.payload_length = payload_len;
    int ret = protocol_header_serialize(&h, buffer, buf_size);
    if (ret != PROTO_OK) return ret;

    /* 拷贝 Payload */
    if (payload_len > 0 && payload) {
        memcpy(buffer + PROTOCOL_HEADER_LENGTH, payload, payload_len);
    }

    /* 计算并填入 Payload CRC16 */
    uint16_t crc = crc16_compute(payload ? payload : (const uint8_t *)"", payload_len);
    buffer[PROTOCOL_HEADER_LENGTH + payload_len]     = (uint8_t)(crc);
    buffer[PROTOCOL_HEADER_LENGTH + payload_len + 1] = (uint8_t)(crc >> 8);

    if (out_len) *out_len = total;
    return PROTO_OK;
}

/* ---- Header 反序列化 ---- */
int protocol_header_deserialize(const uint8_t *buffer, size_t buf_size,
                                 ProtocolHeader *header) {
    if (!buffer || !header || buf_size < PROTOCOL_HEADER_LENGTH)
        return PROTO_ERR_BUFFER_FULL;

    /* CRC8 校验 */
    if (crc8_verify_header(buffer) != 0)
        return PROTO_ERR_CRC_HEADER;

    header->sync            = (uint16_t)(buffer[0] | (buffer[1] << 8));
    header->proto_version   = buffer[2];
    header->header_length   = buffer[3];
    header->msg_type        = buffer[4];
    header->msg_subtype     = buffer[5];
    header->src_node_id     = buffer[6];
    header->dst_node_id     = buffer[7];
    header->mcu_role        = buffer[8];
    header->session_id      = (uint32_t)(buffer[9]  | (buffer[10] << 8) |
                                         (buffer[11] << 16) | (buffer[12] << 24));
    header->sequence_number = (uint32_t)(buffer[13] | (buffer[14] << 8) |
                                         (buffer[15] << 16) | (buffer[16] << 24));
    header->timestamp_ms    = (uint32_t)(buffer[17] | (buffer[18] << 8) |
                                         (buffer[19] << 16) | (buffer[20] << 24));
    header->cmd_ttl_ms      = (uint16_t)(buffer[21] | (buffer[22] << 8));
    header->payload_length  = (uint16_t)(buffer[23] | (buffer[24] << 8));
    header->flags           = (uint16_t)(buffer[25] | (buffer[26] << 8));
    header->fragment_index  = (uint16_t)(buffer[27] | (buffer[28] << 8));
    header->fragment_total  = buffer[29];
    header->header_crc8     = buffer[30];

    /* 同步字检查 */
    if (header->sync != PROTOCOL_SYNC_WORD)
        return PROTO_ERR_SYNC_LOST;

    /* 版本检查 */
    if (header->proto_version != PROTOCOL_CURRENT_VERSION)
        return PROTO_ERR_INVALID_VERSION;

    return PROTO_OK;
}

/* ---- 完整帧反序列化 ---- */
int protocol_frame_deserialize(const uint8_t *buffer, size_t buf_size,
                                ProtocolHeader *header,
                                const uint8_t **payload, uint16_t *payload_len) {
    if (!buffer || !header) return PROTO_ERR_PARAM_ERR;

    int ret = protocol_header_deserialize(buffer, buf_size, header);
    if (ret != PROTO_OK) return ret;

    uint16_t plen = header->payload_length;
    size_t expected = PROTOCOL_HEADER_LENGTH + plen + 2;
    if (buf_size < expected) return PROTO_ERR_INVALID_LENGTH;

    if (plen > 0) {
        /* 校验 Payload CRC16 */
        const uint8_t *p = buffer + PROTOCOL_HEADER_LENGTH;
        uint16_t computed_crc = crc16_compute(p, plen);
        uint16_t received_crc = (uint16_t)(buffer[PROTOCOL_HEADER_LENGTH + plen] |
                                           (buffer[PROTOCOL_HEADER_LENGTH + plen + 1] << 8));
        if (computed_crc != received_crc)
            return PROTO_ERR_CRC_PAYLOAD;

        if (payload) *payload = p;
    } else {
        if (payload) *payload = NULL;
    }

    if (payload_len) *payload_len = plen;
    return PROTO_OK;
}

/* ---- 同步搜索 ---- */
int protocol_find_sync(const uint8_t *buffer, size_t buf_size) {
    for (size_t i = 0; i < buf_size - 1; i++) {
        uint16_t word = (uint16_t)(buffer[i] | (buffer[i + 1] << 8));
        if (word == PROTOCOL_SYNC_WORD) return (int)i;
    }
    return -1;
}

/* ---- 控制命令校验 ---- */
int protocol_validate_command(const ProtocolHeader *header,
                               uint32_t last_accepted_seq,
                               uint32_t last_accepted_timestamp,
                               uint32_t current_time_ms,
                               uint32_t active_session_id) {
    if (!header) return PROTO_ERR_PARAM_ERR;

    /* 1. 会话检查 */
    if (header->session_id != active_session_id)
        return PROTO_ERR_SESSION_INVALID;

    /* 2. 序列号检查 (单调递增, 只接收更新的) */
    if (header->sequence_number <= last_accepted_seq)
        return PROTO_ERR_SEQ_OUTDATED;

    /* 3. 时间戳检查 */
    if (header->timestamp_ms < last_accepted_timestamp)
        return PROTO_ERR_SEQ_OUTDATED;

    /* 4. 有效期检查 */
    if (header->cmd_ttl_ms > 0) {
        uint32_t elapsed = current_time_ms - header->timestamp_ms;
        if (elapsed > header->cmd_ttl_ms)
            return PROTO_ERR_TTL_EXPIRED;
    }

    return PROTO_OK;
}

/* ---- 初始化 ---- */
void protocol_header_init(ProtocolHeader *header, uint8_t msg_type,
                           uint8_t src_node, uint8_t dst_node,
                           uint8_t mcu_role) {
    memset(header, 0, sizeof(*header));
    header->sync = PROTOCOL_SYNC_WORD;
    header->proto_version = PROTOCOL_CURRENT_VERSION;
    header->header_length = PROTOCOL_HEADER_LENGTH;
    header->msg_type = msg_type;
    header->src_node_id = src_node;
    header->dst_node_id = dst_node;
    header->mcu_role = mcu_role;
}

/* ---- 消息类型名称 ---- */
const char *protocol_msg_type_name(uint8_t msg_type) {
    switch (msg_type) {
        case MSG_HEARTBEAT:         return "HEARTBEAT";
        case MSG_CAPABILITY_QUERY:  return "CAPABILITY_QUERY";
        case MSG_VERSION_QUERY:     return "VERSION_QUERY";
        case MSG_REALTIME_CONTROL:  return "REALTIME_CONTROL";
        case MSG_REALTIME_STATE:    return "REALTIME_STATE";
        case MSG_DEVICE_STATE:      return "DEVICE_STATE";
        case MSG_FAULT_EVENT:       return "FAULT_EVENT";
        case MSG_PARAM_READ:        return "PARAM_READ";
        case MSG_PARAM_WRITE:       return "PARAM_WRITE";
        case MSG_PARAM_SYNC:        return "PARAM_SYNC";
        case MSG_TIME_SYNC:         return "TIME_SYNC";
        case MSG_LOG_DATA:          return "LOG_DATA";
        case MSG_DEBUG_CMD:         return "DEBUG_CMD";
        case MSG_FIRMWARE_UPGRADE:  return "FIRMWARE_UPGRADE";
        case MSG_BOOTLOADER_CTRL:   return "BOOTLOADER_CTRL";
        case MSG_LINK_SWITCH:       return "LINK_SWITCH";
        case MSG_SESSION_ESTABLISH: return "SESSION_ESTABLISH";
        case MSG_SESSION_TERMINATE: return "SESSION_TERMINATE";
        case MSG_SAFETY_NOTIFY:     return "SAFETY_NOTIFY";
        case MSG_IDLE_FRAME:        return "IDLE_FRAME";
        default:                    return "UNKNOWN";
    }
}
