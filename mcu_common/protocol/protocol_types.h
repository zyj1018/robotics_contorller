/* 协议类型定义 — RK3588 和 STM32 共享 */
#ifndef PROTOCOL_TYPES_H
#define PROTOCOL_TYPES_H

#include <stdint.h>

/* ---- 协议常量 ---- */
#define PROTOCOL_SYNC_WORD      0xA55A
#define PROTOCOL_HEADER_LENGTH  32
#define PROTOCOL_MAX_PAYLOAD    65535
#define PROTOCOL_CURRENT_VERSION 1

/* ---- 节点 ID ---- */
#define NODE_ID_RK      0x00
#define NODE_ID_MCU_A   0x01
#define NODE_ID_MCU_B   0x02
#define NODE_ID_BROADCAST 0xFF

/* ---- MCU 角色 ---- */
enum {
    MCU_ROLE_RK              = 0,
    MCU_ROLE_CHASSIS         = 1,  /* MCU-A: 底盘域 */
    MCU_ROLE_MANIPULATOR     = 2,  /* MCU-B: 机械臂/上装域 */
};

/* ---- 消息类型 ---- */
enum {
    MSG_HEARTBEAT            = 0x01,
    MSG_CAPABILITY_QUERY     = 0x02,
    MSG_VERSION_QUERY        = 0x03,
    MSG_REALTIME_CONTROL     = 0x10,
    MSG_REALTIME_STATE       = 0x11,
    MSG_DEVICE_STATE         = 0x12,
    MSG_FAULT_EVENT          = 0x20,
    MSG_PARAM_READ           = 0x30,
    MSG_PARAM_WRITE          = 0x31,
    MSG_PARAM_SYNC           = 0x32,
    MSG_TIME_SYNC            = 0x40,
    MSG_LOG_DATA             = 0x50,
    MSG_DEBUG_CMD            = 0x60,
    MSG_FIRMWARE_UPGRADE     = 0x70,
    MSG_BOOTLOADER_CTRL      = 0x71,
    MSG_LINK_SWITCH          = 0x80,
    MSG_SESSION_ESTABLISH    = 0x81,
    MSG_SESSION_TERMINATE    = 0x82,
    MSG_SAFETY_NOTIFY        = 0x90,
    MSG_IDLE_FRAME           = 0x00,  /* 无数据填充帧 */
};

/* ---- 消息子类型 ---- */
/* 实时控制子类型 */
enum {
    REALTIME_SUB_MOTION_CMD   = 0x01,
    REALTIME_SUB_ODOMETRY     = 0x02,
    REALTIME_SUB_ARM_CMD      = 0x03,  /* V2: 机械臂控制 */
};

/* 故障事件子类型 */
enum {
    FAULT_SUB_OCCURRED        = 0x01,
    FAULT_SUB_CLEARED         = 0x02,
    FAULT_SUB_LATCHED         = 0x03,
};

/* 安全通知子类型 */
enum {
    SAFETY_SUB_ESTOP          = 0x01,
    SAFETY_SUB_FAULT          = 0x02,
    SAFETY_SUB_DEGRADED       = 0x03,
    SAFETY_SUB_RECOVERED      = 0x04,
    SAFETY_SUB_SUBSYS_ESTOP   = 0x05,  /* V2: 子系统急停 */
};

/* ---- 控制模式 ---- */
enum {
    CONTROL_MODE_DISABLED     = 0,
    CONTROL_MODE_VELOCITY     = 1,
    CONTROL_MODE_POSITION     = 2,
    CONTROL_MODE_TORQUE       = 3,
    CONTROL_MODE_HOMING       = 4,
};

/* ---- 帧标志位 ---- */
#define FLAG_ACK_REQUESTED     (1 << 0)
#define FLAG_ACK_RESPONSE      (1 << 1)
#define FLAG_URGENT            (1 << 2)
#define FLAG_FRAGMENTED        (1 << 3)
#define FLAG_FRAGMENT_LAST     (1 << 4)
#define FLAG_ENCRYPTED         (1 << 5)
#define FLAG_FROM_MAINTENANCE  (1 << 6)  /* USB/维护链路来源 */

/* ---- 协议错误码 ---- */
enum {
    PROTO_OK                  = 0,
    PROTO_ERR_CRC_HEADER      = -1,
    PROTO_ERR_CRC_PAYLOAD     = -2,
    PROTO_ERR_SYNC_LOST       = -3,
    PROTO_ERR_INVALID_VERSION = -4,
    PROTO_ERR_INVALID_LENGTH  = -5,
    PROTO_ERR_BUFFER_FULL     = -6,
    PROTO_ERR_SEQ_OUTDATED    = -7,
    PROTO_ERR_SESSION_INVALID = -8,
    PROTO_ERR_TTL_EXPIRED     = -9,
    PROTO_ERR_WRONG_TARGET    = -10,
    PROTO_ERR_NO_AUTHORITY    = -11,
    PROTO_ERR_PARAM_ERR       = -12,
    PROTO_ERR                 = -99,
};
#endif /* PROTOCOL_TYPES_H */
