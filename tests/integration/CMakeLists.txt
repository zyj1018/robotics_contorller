add_executable(test_spi_full_duplex test_spi_full_duplex.c)
target_include_directories(test_spi_full_duplex PRIVATE
    ${CMAKE_SOURCE_DIR}/mcu_common
    ${CMAKE_SOURCE_DIR}/mcu_common/protocol
    ${CMAKE_SOURCE_DIR}/mcu_common/transport
    ${CMAKE_SOURCE_DIR}/tests/mock
)
target_sources(test_spi_full_duplex PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/mock/mock_spi.c
    ${CMAKE_SOURCE_DIR}/tests/mock/mock_hal.c
)
target_link_libraries(test_spi_full_duplex mcu_common)
add_test(NAME spi_full_duplex COMMAND test_spi_full_duplex)
