# ============================================================
# ARM Cortex-M4 交叉编译工具链 (STM32G474)
# 用法: cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain_arm_gcc.cmake
#
# 前提: 安装 arm-none-eabi-gcc 工具链
#   sudo apt install gcc-arm-none-eabi
# ============================================================

# ---- 工具链前缀 ----
set(TOOLCHAIN_PREFIX arm-none-eabi)

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_AR           ${TOOLCHAIN_PREFIX}-ar)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}-objcopy)
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}-objdump)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}-size)
set(CMAKE_RANLIB       ${TOOLCHAIN_PREFIX}-ranlib)
set(CMAKE_STRIP        ${TOOLCHAIN_PREFIX}-strip)

# ---- 目标平台 ----
set(TARGET_PLATFORM "ARM" CACHE STRING "" FORCE)
set(OSAL_BACKEND "FREERTOS" CACHE STRING "" FORCE)
set(DRIVER_BACKEND "HAL" CACHE STRING "" FORCE)

# ---- CPU 编译选项 ----
# STM32G474: Cortex-M4 + FPU (single precision)
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS   "${CPU_FLAGS} -fdata-sections -ffunction-sections" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CPU_FLAGS} -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "${CPU_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -specs=nosys.specs -specs=nano.specs" CACHE STRING "" FORCE)

# ---- 查找策略(不使用宿主系统库) ----
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
