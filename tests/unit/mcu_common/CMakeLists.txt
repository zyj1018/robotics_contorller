add_executable(test_protocol_codec test_protocol_codec.c)
target_include_directories(test_protocol_codec PRIVATE ${CMAKE_SOURCE_DIR}/mcu_common ${CMAKE_SOURCE_DIR}/mcu_common/protocol)
target_link_libraries(test_protocol_codec mcu_common)
add_test(NAME protocol_codec COMMAND test_protocol_codec)

add_executable(test_protocol_crc test_protocol_crc.c)
target_include_directories(test_protocol_crc PRIVATE ${CMAKE_SOURCE_DIR}/mcu_common ${CMAKE_SOURCE_DIR}/mcu_common/protocol)
target_link_libraries(test_protocol_crc mcu_common)
add_test(NAME protocol_crc COMMAND test_protocol_crc)

add_executable(test_protocol_session test_protocol_session.c)
target_include_directories(test_protocol_session PRIVATE ${CMAKE_SOURCE_DIR}/mcu_common ${CMAKE_SOURCE_DIR}/mcu_common/protocol)
target_link_libraries(test_protocol_session mcu_common)
add_test(NAME protocol_session COMMAND test_protocol_session)
