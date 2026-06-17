# ============================================================
# RK3588 + 双 STM32G474 机器人控制器 — 顶层 CMake
# ============================================================
cmake_minimum_required(VERSION 3.20)
project(robot_controller VERSION 0.1.0 LANGUAGES C CXX)

# ---- 编译选项 ----
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 目标平台: PC (默认) / ARM ----
set(TARGET_PLATFORM "PC" CACHE STRING "Target platform: PC or ARM")
set_property(CACHE TARGET_PLATFORM PROPERTY STRINGS "PC" "ARM")

if(TARGET_PLATFORM STREQUAL "ARM")
    set(OSAL_BACKEND "FREERTOS")
    set(DRIVER_BACKEND "HAL")
    message(STATUS "Target: ARM (FreeRTOS + HAL)")
else()
    set(OSAL_BACKEND "POSIX")
    set(DRIVER_BACKEND "MOCK")
    message(STATUS "Target: PC (POSIX + Mock)")
endif()

# ---- 全局编译选项 ----
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_RK3588 "Build RK3588 side" ON)
option(BUILD_MCU_A "Build MCU-A firmware" ON)
option(BUILD_MCU_B "Build MCU-B firmware" ON)
option(ENABLE_WERROR "Treat warnings as errors" ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wshadow -Wundef -Wunused -Wconversion)
    if(ENABLE_WERROR)
        add_compile_options(-Werror)
    endif()
    # C-only 警告 (不应用于 C++)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes -Wstrict-prototypes")
    # 调试符号
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
endif()

# ---- 全局包含路径 ----
include_directories(${CMAKE_SOURCE_DIR})

# ---- 子目录 ----
add_subdirectory(mcu_common)

if(BUILD_MCU_A)
    add_subdirectory(mcu_a)
endif()

if(BUILD_MCU_B)
    add_subdirectory(mcu_b)
endif()

if(BUILD_RK3588)
    add_subdirectory(rk3588)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
