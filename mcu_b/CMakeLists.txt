cmake_minimum_required(VERSION 3.22)

if(NOT TARGET_PLATFORM STREQUAL "ARM")
    message(STATUS "MCU-B: skipped (ARM only)")
    return()
endif()

set(THIRD ${CMAKE_SOURCE_DIR}/third_party)
set(BOARD ${CMAKE_CURRENT_SOURCE_DIR}/board)

set(HAL_SOURCES
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc_ex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_flash_ramfunc.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_exti.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma_ex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pwr_ex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_fdcan.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_uart.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_uart_ex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_spi.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_spi_ex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_tim_ex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pcd.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_pcd_ex.c
    ${THIRD}/STM32G4xx_HAL_Driver/Src/stm32g4xx_ll_usb.c
)

set(FREERTOS_SOURCES
    ${THIRD}/FreeRTOS/Source/croutine.c
    ${THIRD}/FreeRTOS/Source/event_groups.c
    ${THIRD}/FreeRTOS/Source/list.c
    ${THIRD}/FreeRTOS/Source/queue.c
    ${THIRD}/FreeRTOS/Source/stream_buffer.c
    ${THIRD}/FreeRTOS/Source/tasks.c
    ${THIRD}/FreeRTOS/Source/timers.c
    ${THIRD}/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
    ${THIRD}/FreeRTOS/Source/portable/MemMang/heap_4.c
    ${THIRD}/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
)

set(USB_SOURCES
    ${THIRD}/STM32_USB_Device_Library/Core/Src/usbd_core.c
    ${THIRD}/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
    ${THIRD}/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
    ${THIRD}/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
)

set(BOARD_SOURCES
    ${BOARD}/stm32g4xx_it.c
    ${BOARD}/stm32g4xx_hal_msp.c
    ${BOARD}/stm32g4xx_hal_timebase_tim.c
    ${BOARD}/system_stm32g4xx.c
    ${BOARD}/sysmem.c
    ${BOARD}/syscalls.c
    ${BOARD}/startup_stm32g474xx.s
    ${BOARD}/peripherals.c
    ${BOARD}/usb_device.c
    ${BOARD}/usbd_desc.c
    ${BOARD}/usbd_cdc_if.c
    ${BOARD}/usbd_conf.c
)

set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/application/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/application/app_freertos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/application/app_tasks.c
)

add_executable(mcu_b_firmware
    ${HAL_SOURCES} ${FREERTOS_SOURCES} ${USB_SOURCES}
    ${BOARD_SOURCES} ${APP_SOURCES}
)

target_include_directories(mcu_b_firmware PRIVATE
    ${BOARD}
    ${THIRD}/STM32G4xx_HAL_Driver/Inc
    ${THIRD}/STM32G4xx_HAL_Driver/Inc/Legacy
    ${THIRD}/CMSIS/Device/ST/STM32G4xx/Include
    ${THIRD}/CMSIS/Include
    ${THIRD}/FreeRTOS/Source/include
    ${THIRD}/FreeRTOS/Source/CMSIS_RTOS_V2
    ${THIRD}/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ${THIRD}/STM32_USB_Device_Library/Core/Inc
    ${THIRD}/STM32_USB_Device_Library/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/mcu_common
    ${CMAKE_SOURCE_DIR}/mcu_common/protocol
    ${CMAKE_SOURCE_DIR}/mcu_common/drivers
)

target_compile_definitions(mcu_b_firmware PRIVATE
    USE_HAL_DRIVER STM32G474xx IS_MCU_MANIPULATOR=1
)

target_link_libraries(mcu_b_firmware mcu_common)

target_link_options(mcu_b_firmware PRIVATE
    -T${BOARD}/STM32G474XX_FLASH.ld
    -Wl,--gc-sections
)

# Post-build: generate .bin and .hex
add_custom_command(TARGET mcu_b_firmware POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:mcu_b_firmware> ${CMAKE_CURRENT_BINARY_DIR}/mcu_b_firmware.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:mcu_b_firmware> ${CMAKE_CURRENT_BINARY_DIR}/mcu_b_firmware.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:mcu_b_firmware>
    COMMENT "Generating .bin and .hex for MCU-B"
)
