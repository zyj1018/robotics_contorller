# ============================================================
# PC 本地编译工具链 (Linux x86_64)
# 用法: cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain_pc.cmake
# ============================================================
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_AR           gcc-ar)
set(CMAKE_RANLIB       gcc-ranlib)

set(TARGET_PLATFORM "PC" CACHE STRING "" FORCE)
set(OSAL_BACKEND "POSIX" CACHE STRING "" FORCE)
set(DRIVER_BACKEND "MOCK" CACHE STRING "" FORCE)
