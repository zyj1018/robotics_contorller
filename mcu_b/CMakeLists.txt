if(TARGET_PLATFORM STREQUAL "ARM")
  add_executable(mcu_b_firmware application/main.c)
  target_link_libraries(mcu_b_firmware mcu_common)
endif()
