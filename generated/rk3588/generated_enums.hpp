// Auto-generated C++ definitions — DO NOT EDIT
// Generated: 2026-06-17T15:07:28.697762
#pragma once
#include <cstdint>

namespace robot_controller { namespace generated {

enum class ControlMode : uint8_t {
    DISABLED = 0,
    VELOCITY = 1,
    POSITION = 2,
    TORQUE = 3,
    HOMING = 4
};

enum class FaultLevel : uint8_t {
    INFO = 0,
    WARNING = 1,
    DEGRADED = 2,
    ERROR = 3,
    CRITICAL = 4
};

enum class McuRole : uint8_t {
    RK = 0,
    CHASSIS = 1,
    MANIPULATOR = 2
};

enum class SafetyState : uint8_t {
    POWER_OFF = 0,
    INIT = 1,
    SELF_TEST = 2,
    STANDBY = 3,
    READY = 4,
    RUNNING = 5,
    DEGRADED = 6,
    FAULT = 7,
    EMERGENCY_STOP = 8,
    UPDATE = 9,
    RECOVERY = 10
};

}} // namespace robot_controller::generated
