# MCU-A: 底盘域 — ARM Cortex-M4 + FreeRTOS
cmake_minimum_required(VERSION 3.22)

if(NOT TARGET_PLATFORM STREQUAL "ARM")
    message(STATUS "MCU-A: skipped (ARM only)")
    return()
endif()

set(CUBEMX ${CMAKE_SOURCE_DIR}/mcu_a_cubemx/mcu_A)

# ---- CubeMX 应用源文件 ----
set(MCU_A_SOURCES
    ${CUBEMX}/Core/Src/stm32g4xx_it.c
    ${CUBEMX}/Core/Src/stm32g4xx_hal_msp.c
    ${CUBEMX}/Core/Src/stm32g4xx_hal_timebase_tim.c
    ${CUBEMX}/Core/Src/sysmem.c
    ${CUBEMX}/Core/Src/syscalls.c
    ${CUBEMX}/Core/Src/system_stm32g4xx.c
    ${CUBEMX}/startup_stm32g474xx.s

    # USB CDC
    ${CUBEMX}/USB_Device/App/usb_device.c
    ${CUBEMX}/USB_Device/App/usbd_desc.c
    ${CUBEMX}/USB_Device/App/usbd_cdc_if.c
    ${CUBEMX}/USB_Device/Target/usbd_conf.c
    ${CUBEMX}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
    ${CUBEMX}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
    ${CUBEMX}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
    ${CUBEMX}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c

    # STM32 HAL
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_exti.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_fdcan.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_uart.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_uart_ex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_spi.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_spi_ex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim_ex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pcd.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pcd_ex.c
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_ll_usb.c

    # FreeRTOS
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/croutine.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/list.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/queue.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/tasks.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/timers.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c

    # MCU-A 业务代码
    ${CMAKE_CURRENT_SOURCE_DIR}/application/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/application/app_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/application/app_freertos.c
)

add_executable(mcu_a_firmware ${MCU_A_SOURCES})

target_include_directories(mcu_a_firmware PRIVATE
    ${CUBEMX}/Core/Inc
    ${CUBEMX}/USB_Device/App
    ${CUBEMX}/USB_Device/Target
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Inc
    ${CUBEMX}/Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
    ${CUBEMX}/Drivers/CMSIS/Device/ST/STM32G4xx/Include
    ${CUBEMX}/Drivers/CMSIS/Include
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/include
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    ${CUBEMX}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ${CUBEMX}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${CUBEMX}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/mcu_common
    ${CMAKE_SOURCE_DIR}/mcu_common/protocol
    ${CMAKE_SOURCE_DIR}/mcu_common/drivers
)

target_compile_definitions(mcu_a_firmware PRIVATE
    USE_HAL_DRIVER
    STM32G474xx
    IS_MCU_CHASSIS=1
)

target_link_libraries(mcu_a_firmware mcu_common)

target_link_options(mcu_a_firmware PRIVATE
    -T${CUBEMX}/STM32G474XX_FLASH.ld
    -Wl,--gc-sections
)
