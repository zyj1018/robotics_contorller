/* Auto-generated message definitions — DO NOT EDIT */
/* Generated: 2026-06-17T15:07:28.697644 */
#ifndef GENERATED_MESSAGES_H
#define GENERATED_MESSAGES_H
#include <stdint.h>
#include "protocol_frame.h"

/* 运动控制命令 */
#define MSG_ID_MOTIONCOMMAND 0x1001
typedef struct {
    uint8_t control_mode;  /* 控制模式 */
    float target_velocity;  /* 目标速度 m/s */
    float target_position;  /* 目标位置 m */
    float target_torque;  /* 目标力矩 Nm */
    float acceleration_limit;  /* 加速度限制 */
    float deceleration_limit;  /* 减速度限制 */
} MotionCommand;

/* 运动状态反馈 */
#define MSG_ID_MOTIONSTATE 0x1002
typedef struct {
    float current_velocity;  /* 当前速度 */
    float current_position;  /* 当前位置 */
    float current_torque;  /* 当前力矩 */
    float motor_temp;  /* 电机温度 ℃ */
    uint16_t fault_code;  /* 故障码 */
    uint16_t status_flags;  /* 状态标志位 */
} MotionState;

#endif /* GENERATED_MESSAGES_H */
