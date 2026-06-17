/* 协议编解码 — 跨平台 C API */
#ifndef PROTOCOL_CODEC_H
#define PROTOCOL_CODEC_H
#include "protocol_frame.h"
#include <stddef.h>

/* ---- 序列化 ---- */

/* 将 Header 序列化到 32 字节 buffer (含 CRC8) */
int protocol_header_serialize(const ProtocolHeader *header, uint8_t *buffer, size_t buf_size);

/* 序列化完整帧: Header + Payload + Payload CRC16 → buffer */
int protocol_frame_serialize(const ProtocolHeader *header,
                              const uint8_t *payload, uint16_t payload_len,
                              uint8_t *buffer, size_t buf_size, size_t *out_len);

/* ---- 反序列化 ---- */

/* 从 buffer 反序列化 Header (含 CRC8 校验) */
int protocol_header_deserialize(const uint8_t *buffer, size_t buf_size,
                                 ProtocolHeader *header);

/* 从 buffer 反序列化完整帧 (Header + Payload + CRC16 校验) */
int protocol_frame_deserialize(const uint8_t *buffer, size_t buf_size,
                                ProtocolHeader *header,
                                const uint8_t **payload, uint16_t *payload_len);

/* ---- 帧同步搜索 ---- */

/* 在 buffer 中搜索同步字 0xA55A，返回偏移量，-1=未找到 */
int protocol_find_sync(const uint8_t *buffer, size_t buf_size);

/* ---- 校验 ---- */

/* 验证控制命令有效性：序列号、时间戳、TTL、会话ID */
int protocol_validate_command(const ProtocolHeader *header,
                               uint32_t last_accepted_seq,
                               uint32_t last_accepted_timestamp,
                               uint32_t current_time_ms,
                               uint32_t active_session_id);

/* ---- 工具 ---- */

/* 初始化一个 Header 到合理默认值 */
void protocol_header_init(ProtocolHeader *header, uint8_t msg_type,
                           uint8_t src_node, uint8_t dst_node,
                           uint8_t mcu_role);

/* 获取人类可读的消息类型名称 */
const char *protocol_msg_type_name(uint8_t msg_type);

#endif /* PROTOCOL_CODEC_H */
