set(MCU_COMMON_SOURCES
    protocol/protocol_codec.c
    protocol/protocol_session.c
    protocol/protocol_crc.c
)

set(MCU_COMMON_HEADERS
    osal/osal.h osal/osal_types.h osal/osal_task.h
    osal/osal_queue.h osal/osal_semaphore.h osal/osal_event.h osal/osal_timer.h
    protocol/protocol_frame.h protocol/protocol_codec.h protocol/protocol_types.h
    protocol/protocol_session.h protocol/protocol_crc.h
    drivers/canfd_driver.h drivers/rs485_driver.h drivers/spi_slave_driver.h
)

add_library(mcu_common STATIC ${MCU_COMMON_SOURCES} ${MCU_COMMON_HEADERS})
target_include_directories(mcu_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(OSAL_BACKEND STREQUAL "FREERTOS")
    target_compile_definitions(mcu_common PUBLIC OSAL_BACKEND_FREERTOS=1)
    target_sources(mcu_common PRIVATE
        drivers/canfd_driver.c
        drivers/rs485_driver.c
        drivers/spi_slave_driver.c
    )
else()
    target_compile_definitions(mcu_common PUBLIC OSAL_BACKEND_POSIX=1)
    target_link_libraries(mcu_common PUBLIC pthread)
    target_sources(mcu_common PRIVATE
        drivers/canfd_driver.c
        drivers/rs485_driver.c
        drivers/spi_slave_driver.c
    )
endif()

if(DRIVER_BACKEND STREQUAL "MOCK")
    target_compile_definitions(mcu_common PUBLIC DRIVER_BACKEND_MOCK=1)
    target_include_directories(mcu_common PUBLIC ${CMAKE_SOURCE_DIR}/tests/mock)
endif()
