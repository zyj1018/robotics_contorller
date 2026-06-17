/* 协议帧结构定义 — 跨平台共享 */
#ifndef PROTOCOL_FRAME_H
#define PROTOCOL_FRAME_H
#include "protocol_types.h"

/* 编译器和平台适配 */
#if defined(__GNUC__) || defined(__clang__)
  #define PACKED_STRUCT __attribute__((packed))
  #define PACKED_ENUM  __attribute__((packed))
#elif defined(_MSC_VER)
  #define PACKED_STRUCT
  #define PACKED_ENUM
  #pragma pack(push, 1)
#else
  #define PACKED_STRUCT
  #define PACKED_ENUM
#endif

/* ---- 协议帧头 (32 字节, packed) ---- */
typedef struct PACKED_STRUCT {
    uint16_t sync;              /* +0: 同步字 0xA55A */
    uint8_t  proto_version;     /* +2: 协议版本 */
    uint8_t  header_length;     /* +3: Header 长度 (固定32) */
    uint8_t  msg_type;          /* +4: 消息类型 */
    uint8_t  msg_subtype;       /* +5: 消息子类型 */
    uint8_t  src_node_id;       /* +6: 源节点 ID */
    uint8_t  dst_node_id;       /* +7: 目标节点 ID */
    uint8_t  mcu_role;          /* +8: MCU 角色 */
    uint32_t session_id;        /* +9: 会话 ID */
    uint32_t sequence_number;   /* +13: 序列号 (单调递增) */
    uint32_t timestamp_ms;      /* +17: 发送时间戳 (ms) */
    uint16_t cmd_ttl_ms;        /* +21: 控制命令有效期 (ms) */
    uint16_t payload_length;    /* +23: Payload 长度 (字节) */
    uint16_t flags;             /* +25: 标志位 */
    uint16_t fragment_index;    /* +27: 分片序号 */
    uint8_t  fragment_total;    /* +29: 分片总数 (0=未分片) */
    uint8_t  reserved;          /* +30: 保留 */
    uint8_t  header_crc8;       /* +31: Header CRC-8 */
    /* Total: 32 bytes */
} ProtocolHeader;

/* 编译时断言 Header 大小 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(ProtocolHeader) == 32, "ProtocolHeader must be 32 bytes");
#endif

/* ---- 简单消息结构 ---- */
typedef struct {
    ProtocolHeader header;
    uint8_t        payload[PROTOCOL_MAX_PAYLOAD];
    uint16_t       payload_crc16;
} ProtocolMessage;

/* ---- 版本信息 ---- */
typedef struct PACKED_STRUCT {
    uint8_t  proto_major;
    uint8_t  proto_minor;
    uint16_t fw_major;
    uint16_t fw_minor;
    uint16_t fw_patch;
    uint32_t fw_build;
    uint8_t  bootloader_major;
    uint8_t  bootloader_minor;
    uint8_t  hw_revision;
    uint8_t  reserved[3];
} VersionInfo;

/* ---- MCU 能力描述 ---- */
typedef struct PACKED_STRUCT {
    uint8_t  mcu_role;
    uint8_t  canfd_channel_count;
    uint8_t  rs485_channel_count;
    uint8_t  max_devices_per_can;
    uint8_t  max_devices_per_rs485;
    uint16_t max_control_frequency_hz;
    uint32_t feature_flags;
    uint32_t max_payload_size;
    uint8_t  reserved[16];
} McuCapabilities;

/* ---- 心跳消息 ---- */
typedef struct PACKED_STRUCT {
    uint32_t uptime_ms;
    uint8_t  safety_state;
    uint8_t  link_state;
    uint16_t error_count;
    uint8_t  reserved[8];
} HeartbeatPayload;

/* ---- 时间同步消息 ---- */
typedef struct PACKED_STRUCT {
    uint32_t t1_master_send;
    uint32_t t2_slave_recv;
    uint32_t t3_slave_send;
    uint32_t rtt_us;
    int32_t  offset_us;
} TimeSyncPayload;

/* 恢复默认对齐 */
#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#undef PACKED_STRUCT
#undef PACKED_ENUM
#endif /* PROTOCOL_FRAME_H */
