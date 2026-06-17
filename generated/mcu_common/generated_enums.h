/* Auto-generated enum definitions — DO NOT EDIT */
/* Generated: 2026-06-17T15:07:28.697539 */
#ifndef GENERATED_ENUMS_H
#define GENERATED_ENUMS_H
#include <stdint.h>

/* ControlMode */
#define ControlMode_DISABLED 0
#define ControlMode_VELOCITY 1
#define ControlMode_POSITION 2
#define ControlMode_TORQUE 3
#define ControlMode_HOMING 4

/* FaultLevel */
#define FaultLevel_INFO 0
#define FaultLevel_WARNING 1
#define FaultLevel_DEGRADED 2
#define FaultLevel_ERROR 3
#define FaultLevel_CRITICAL 4

/* McuRole */
#define McuRole_RK 0
#define McuRole_CHASSIS 1
#define McuRole_MANIPULATOR 2

/* SafetyState */
#define SafetyState_POWER_OFF 0
#define SafetyState_INIT 1
#define SafetyState_SELF_TEST 2
#define SafetyState_STANDBY 3
#define SafetyState_READY 4
#define SafetyState_RUNNING 5
#define SafetyState_DEGRADED 6
#define SafetyState_FAULT 7
#define SafetyState_EMERGENCY_STOP 8
#define SafetyState_UPDATE 9
#define SafetyState_RECOVERY 10

#endif /* GENERATED_ENUMS_H */
