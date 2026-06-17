/* Auto-generated fault codes — DO NOT EDIT */
/* Generated: 2026-06-17T15:07:28.697711 */
#ifndef GENERATED_FAULT_CODES_H
#define GENERATED_FAULT_CODES_H

#define FAULT_CANFD_BUS_OFF 0xA001
#define FAULT_CANFD_ERROR_PASSIVE 0xA002
#define FAULT_CANFD_NODE_OFFLINE 0xA003
#define FAULT_RS485_DEVICE_OFFLINE 0xB001
#define FAULT_RS485_CRC_ERROR_HIGH 0xB002
#define FAULT_MOTOR_OVER_TEMP 0xC001
#define FAULT_MOTOR_OVER_CURRENT 0xC002
#define FAULT_SAFETY_ESTOP_ACTIVATED 0xD001
#define FAULT_SAFETY_WATCHDOG_RESET 0xD002
#define FAULT_COMM_SPI_CRC_ERROR 0xE001
#define FAULT_COMM_HEARTBEAT_LOST 0xE002
#define FAULT_SYSTEM_FW_VERSION_MISMATCH 0xF001

/* Fault code lookup */
typedef struct {
    uint16_t code; const char *module; const char *desc; uint8_t level;
} fault_code_entry_t;

static const fault_code_entry_t fault_code_table[] = {
    {0xA001, "CANFD", "CAN FD Bus Off", 3},
    {0xA002, "CANFD", "CAN FD Error Passive", 1},
    {0xA003, "CANFD", "CAN节点离线", 2},
    {0xB001, "RS485", "RS485设备离线", 3},
    {0xB002, "RS485", "RS485 CRC错误率过高", 2},
    {0xC001, "MOTOR", "电机过温", 3},
    {0xC002, "MOTOR", "电机过流", 4},
    {0xD001, "SAFETY", "急停激活", 4},
    {0xD002, "SAFETY", "看门狗复位", 4},
    {0xE001, "COMM", "SPI CRC错误", 2},
    {0xE002, "COMM", "心跳丢失", 4},
    {0xF001, "SYSTEM", "固件版本不匹配", 4}
};
#endif /* GENERATED_FAULT_CODES_H */
