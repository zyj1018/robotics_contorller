# Agent Code Review Report - codex branch

Date: 2026-06-17
Repository: `E:\ytns_work\robotics_contorller`
Branch: `codex`
Reviewed commit: `cb89cc9 refactor: merge CubeMX projects into unified structure`
Reviewer role: `Reviewer`

## Conclusion

- Result: `REQUEST_CHANGES`
- Risk: `HIGH`
- Merge recommendation: do not merge yet.

The current branch has a verified PC test build failure and a high-confidence MCU firmware build break. The main risk is that both the host-side regression path and the embedded firmware output path are currently not reliable.

## Scope

- Review target: current repository state on `codex`.
- Diff source: full repository review, no staged or worktree diff.
- Primary areas reviewed: CMake build configuration, MCU-A/MCU-B firmware entry points, common protocol codec/session code, CAN FD/RS485/SPI driver skeletons, unit-test configuration.
- Temporary build directory used for evidence: `build-review`, removed after review.

## Requirement Traceability

| ID | Requirement | Module | Implementation | Tests | Status | Evidence |
|---|---|---|---|---|---|---|
| R1 | PC development and tests build successfully | CMake/tests | CMake configure succeeds, build fails | Tests could not run | `INCORRECT` | MSVC link fails on `pthread.lib` |
| R2 | MCU-A/B firmware can be built | MCU CMake/app | Required CubeMX init symbols and HAL handles are missing | ARM toolchain unavailable locally | `INCORRECT` | Symbols only have `extern` declarations |
| R3 | Protocol sync search is safe for partial buffers | `protocol_codec` | `buf_size - 1` underflows for 0-byte buffers | Not covered | `INCORRECT` | `protocol_find_sync` lacks `buf_size < 2` guard |
| R4 | Payload CRC is checked consistently | `protocol_codec` | Empty payload frames skip CRC16 verification | Only normal empty payload covered | `PARTIAL` | Serializer writes CRC16 for all frames, deserializer skips when `plen == 0` |
| R5 | CAN FD length handling matches STM32 HAL API | `canfd_driver` | HAL DLC enum and byte length are mixed | Not covered | `INCORRECT` | `DataLength = frame->dlc` and `frame.dlc = rxh.DataLength` |

## Findings And Fix Suggestions

### ISSUE-001: PC tests fail to link on Windows/MSVC

- Severity: `BLOCKER`
- Confidence: `HIGH`
- Category: build / compatibility
- Location:
  - `mcu_common/CMakeLists.txt:27`
  - `tests/unit/mcu_common/CMakeLists.txt:3`
  - `tests/unit/mcu_common/CMakeLists.txt:8`
  - `tests/unit/mcu_common/CMakeLists.txt:13`
- Evidence:
  - `cmake -S . -B build-review -DTARGET_PLATFORM=PC -DBUILD_TESTS=ON` succeeded.
  - `cmake --build build-review --config Debug` failed with `LINK : fatal error LNK1104: cannot open file 'pthread.lib'`.
- Trigger: build tests on Windows with the default Visual Studio generator.
- Impact: the PC regression path cannot build, so unit tests cannot run in this environment.
- Suggested fix:
  - Replace raw `pthread` linkage with CMake Threads:

```cmake
find_package(Threads REQUIRED)
target_link_libraries(mcu_common PUBLIC Threads::Threads)
```

  - Remove explicit `pthread` from test targets and rely on `mcu_common` transitively, or link `Threads::Threads` directly where needed.
  - For Windows/MSVC, verify whether `mcu_common/osal/osal_posix.c` is intended to compile. If Windows is supported, add a Win32 OSAL backend or use a pthread compatibility layer explicitly.

### ISSUE-002: MCU-A/B firmware is missing CubeMX initialization definitions

- Severity: `BLOCKER`
- Confidence: `HIGH`
- Category: build / embedded startup
- Location:
  - `mcu_a/application/main.c:7`
  - `mcu_a/application/app_tasks.c:8`
  - `mcu_a/CMakeLists.txt:62`
  - same pattern in `mcu_b/application/main.c` and `mcu_b/application/app_tasks.c`
- Evidence:
  - `SystemClock_Config`, `MX_GPIO_Init`, `MX_DMA_Init`, `MX_FDCAN2_Init`, `MX_FDCAN3_Init`, `MX_LPUART1_UART_Init`, `MX_USART1_UART_Init`, `MX_USART3_UART_Init`, `MX_SPI2_Init`, and HAL handles such as `hfdcan2`, `hfdcan3`, `hspi2`, `hlpuart1`, `huart3` are referenced as `extern`.
  - Repository search found declarations and calls, but no definitions for those init functions or handles.
  - `mcu_a/CMakeLists.txt` and `mcu_b/CMakeLists.txt` include board support files but not an equivalent CubeMX `main.c` source containing these definitions.
- Trigger: linking `mcu_a_firmware` or `mcu_b_firmware`.
- Impact: MCU firmware link should fail with undefined symbols; firmware images cannot be produced.
- Suggested fix:
  - Restore the CubeMX-generated initialization source, or split it into board-owned files such as:
    - `mcu_a/board/board_init.c`
    - `mcu_a/board/peripherals.c`
    - `mcu_b/board/board_init.c`
    - `mcu_b/board/peripherals.c`
  - Define all HAL handles in exactly one compilation unit per MCU target.
  - Add the new source files to each MCU `BOARD_SOURCES` list.
  - Keep application `main.c` focused on application startup and FreeRTOS handoff.

### ISSUE-003: Startup assembly is listed without enabling ASM in CMake

- Severity: `MAJOR`
- Confidence: `MEDIUM`
- Category: build / embedded startup
- Location:
  - `CMakeLists.txt:5`
  - `mcu_a/CMakeLists.txt:69`
  - `mcu_b/CMakeLists.txt:64`
- Evidence:
  - Top-level CMake declares `project(robot_controller VERSION 0.1.0 LANGUAGES C CXX)`.
  - MCU targets include `startup_stm32g474xx.s`.
- Trigger: ARM firmware configure/build.
- Impact: the startup assembly may not be compiled as expected, which can remove vector table and `Reset_Handler` from the final firmware.
- Suggested fix:
  - Add `ASM` to the top-level project languages:

```cmake
project(robot_controller VERSION 0.1.0 LANGUAGES C CXX ASM)
```

  - Alternatively call `enable_language(ASM)` only when `TARGET_PLATFORM STREQUAL "ARM"`.
  - Verify the generated build includes `startup_stm32g474xx.s` in the object list.

### ISSUE-004: `protocol_find_sync` can read out of bounds on empty or 1-byte buffers

- Severity: `CRITICAL`
- Confidence: `HIGH`
- Category: correctness / memory safety
- Location: `mcu_common/protocol/protocol_codec.c:152`
- Evidence:
  - Loop condition uses `i < buf_size - 1`.
  - When `buf_size == 0`, unsigned `size_t` subtraction underflows to `SIZE_MAX`.
  - The loop then reads `buffer[i]` and `buffer[i + 1]`.
- Trigger: receive parser tries to resynchronize on an empty or single-byte partial buffer.
- Impact: out-of-bounds read, crash, or false sync detection in the receive path.
- Suggested fix:

```c
int protocol_find_sync(const uint8_t *buffer, size_t buf_size) {
    if (!buffer || buf_size < 2) {
        return -1;
    }
    for (size_t i = 0; i + 1 < buf_size; i++) {
        uint16_t word = (uint16_t)(buffer[i] | (buffer[i + 1] << 8));
        if (word == PROTOCOL_SYNC_WORD) {
            return (int)i;
        }
    }
    return -1;
}
```

  - Add unit tests for `NULL`, `0`, `1`, and `2` byte inputs.

### ISSUE-005: Empty payload frames skip CRC16 verification

- Severity: `MAJOR`
- Confidence: `HIGH`
- Category: correctness / protocol integrity
- Location: `mcu_common/protocol/protocol_codec.c:133`
- Evidence:
  - `protocol_frame_serialize` always writes `Header + Payload + CRC16`, even when `payload_len == 0`.
  - `protocol_frame_deserialize` only verifies payload CRC16 inside `if (plen > 0)`.
- Trigger: heartbeat, empty ACK, idle, or command frames with `payload_length == 0`.
- Impact: corruption of the trailing CRC16 bytes in empty-payload frames is accepted as valid.
- Suggested fix:
  - Always compute and compare CRC16 after length validation:

```c
const uint8_t *p = buffer + PROTOCOL_HEADER_LENGTH;
uint16_t computed_crc = crc16_compute(p, plen);
uint16_t received_crc = (uint16_t)(buffer[PROTOCOL_HEADER_LENGTH + plen] |
                                  (buffer[PROTOCOL_HEADER_LENGTH + plen + 1] << 8));
if (computed_crc != received_crc) {
    return PROTO_ERR_CRC_PAYLOAD;
}
```

  - Add a unit test that serializes an empty payload frame, flips one CRC byte, and expects `PROTO_ERR_CRC_PAYLOAD`.

### ISSUE-006: CAN FD DLC is mixed with byte length

- Severity: `MAJOR`
- Confidence: `HIGH`
- Category: correctness / embedded driver
- Location:
  - `mcu_common/drivers/canfd_driver.c:46`
  - `mcu_common/drivers/canfd_driver.c:84`
- Evidence:
  - `canfd_frame_t.dlc` is represented as a `uint8_t` length-like field next to `data[64]`.
  - STM32 HAL FDCAN `DataLength` expects `FDCAN_DLC_BYTES_*` enum values.
  - Send path assigns `txh.DataLength = frame->dlc`.
  - Receive path assigns `frame.dlc = rxh.DataLength`.
- Trigger: CAN FD frames with payload lengths beyond classic CAN values, or any code that treats `frame.dlc` as bytes.
- Impact: incorrect transmission length and incorrect receive parsing.
- Suggested fix:
  - Decide whether `canfd_frame_t.dlc` stores byte length or raw HAL DLC code. Prefer byte length for driver API clarity.
  - Add conversion helpers:
    - `canfd_len_to_hal_dlc(uint8_t len, uint32_t *out_dlc)`
    - `canfd_hal_dlc_to_len(uint32_t hal_dlc, uint8_t *out_len)`
  - Reject invalid lengths and add unit tests for 0, 8, 12, 16, 20, 24, 32, 48, and 64 bytes.

## Additional Observations

- Several source and Markdown files display garbled Chinese comments in the current terminal/code page, and MSVC emits `C4819` warnings. This is not the direct cause of the failed build, but it makes review and Windows builds noisy. Prefer saving all text files as UTF-8 and setting the compiler/source charset explicitly if Windows support matters.
- `mcu_common/osal/osal_freertos.c` is currently an inactive placeholder guarded by `#if 0`. This is acceptable only if no application code links against OSAL FreeRTOS functions yet. If OSAL is part of the firmware contract, this needs implementation before real firmware integration.
- `canfd_ring_t` is shared between ISR and task code using only `volatile` fields. This should receive a dedicated concurrency review after the build issues are fixed.

## Checks Run

| Check | Command | Status | Exit Code | Evidence |
|---|---|---:|---:|---|
| Skill status | `python C:\Users\12275\.codex\skills\agent-code-review\scripts\agent_code_review.py status` | PASS | 0 | branch `codex`, clean worktree |
| Skill detect | `python C:\Users\12275\.codex\skills\agent-code-review\scripts\agent_code_review.py detect` | PASS | 0 | C/C++/CMake detected |
| Skill tests hint | `python C:\Users\12275\.codex\skills\agent-code-review\scripts\agent_code_review.py tests` | PASS | 0 | helper reported likely test commands |
| Skill security hint | `python C:\Users\12275\.codex\skills\agent-code-review\scripts\agent_code_review.py security` | PASS | 0 | helper reported detected repo metadata |
| PC configure | `cmake -S . -B build-review -DTARGET_PLATFORM=PC -DBUILD_TESTS=ON` | PASS | 0 | Visual Studio 2022 project generated |
| PC build | `cmake --build build-review --config Debug` | FAIL | 1 | `pthread.lib` missing |
| ARM toolchain check | `arm-none-eabi-gcc --version` | FAIL | 1 | command not found |
| Cleanup | remove `build-review` | PASS | 0 | temporary build directory removed |

## Test Quality

The current protocol tests cover normal header/frame round trips, header CRC failure, version mismatch, normal sync search, basic command validation, and normal empty payload handling. Missing tests include:

- `protocol_find_sync(NULL, 0)`, empty buffer, and 1-byte buffer.
- Empty payload frame with corrupted CRC16.
- `payload_len > 0` with `payload == NULL`.
- CAN FD DLC conversion and invalid length rejection.
- RS485 transaction state transitions, timeout/retry paths, and wraparound receive buffer behavior.
- SPI slave transfer completion and null/oversized response handling.

## Unverified Areas

- ARM firmware build was not executed because `arm-none-eabi-gcc` is not installed in the current environment.
- Unit tests were not executed because the PC build failed before producing test binaries.
- Third-party source code under `third_party/` was not reviewed except where it interacts with project CMake and HAL APIs.
- Hardware behavior was not verified.

## Recommended Fix Order

1. Fix CMake thread linkage for PC tests and decide whether Windows/MSVC is a supported PC target.
2. Restore or recreate CubeMX initialization definitions and HAL handle definitions for MCU-A and MCU-B.
3. Enable ASM in CMake for MCU startup files.
4. Fix `protocol_find_sync` bounds handling and add boundary tests.
5. Fix empty payload CRC16 verification and add a corruption test.
6. Fix CAN FD byte-length to HAL-DLC conversion and add tests.
7. Re-run PC build/tests and then run ARM cross build once `arm-none-eabi-gcc` is available.

## Final Recommendation

Current branch should remain in review. The build blockers should be fixed before further functional work is layered on top, because the test and firmware build feedback loops are currently unreliable.
