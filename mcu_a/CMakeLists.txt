if(TARGET_PLATFORM STREQUAL "ARM")
  add_executable(mcu_a_firmware application/main.c)
  target_link_libraries(mcu_a_firmware mcu_common)
endif()
