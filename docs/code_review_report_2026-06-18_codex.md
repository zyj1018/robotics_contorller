# Code Review Report - 2026-06-18

## 1. Conclusion

- Result: `REQUEST_CHANGES`
- Risk: `HIGH`
- Review branch: `codex`
- Base commit: `dd63498`
- Reviewed target: `b61b539`
- Reviewed commits:
  - `80f81b3 feat(p2): implement protocol integration layer`
  - `b61b539 docs: update README with P1/P2 completion status and test coverage`
- Merge recommendation: Do not merge or release until the Transport initialization and session authorization defects are fixed and covered by tests.

The PC build and all configured CTest targets pass. However, the new SPI Transport cannot be used after initialization because its context remains null. The Link Manager also permits real-time control frames without a valid active session. These defects affect primary communication and control paths.

## 2. Scope

The review covered the changes between `dd63498` and `b61b539`, including:

- `mcu_common/protocol/link_manager.c`
- `mcu_common/protocol/link_manager.h`
- `mcu_common/transport/transport_interface.c`
- `mcu_common/transport/transport_interface.h`
- `tests/unit/mcu_common/test_link_manager.c`
- `tests/integration/test_spi_full_duplex.c`
- Related CMake and README changes

The uncommitted change in `mcu_a/application/app_tasks.c` was not part of this review.

## 3. Requirement Traceability

| ID | Requirement | Implementation | Tests | Status |
|---|---|---|---|---|
| R-01 | SPI has primary-link priority | Link selection implemented | Unit test present | `IMPLEMENTED` |
| R-02 | USB cannot issue real-time control | Direct message-type rejection | Unit test present | `IMPLEMENTED` |
| R-03 | USART rescue mode only permits whitelisted commands | Whitelist implemented | Unit test present | `IMPLEMENTED` |
| R-04 | Real-time control requires an active, matching session | Session check is conditional and bypassable | Missing negative tests | `INCORRECT` |
| R-05 | Transport abstraction can initialize and perform SPI send/receive | SPI context is never bound | Transport API not exercised | `INCORRECT` |
| R-06 | Invalid link identifiers are rejected safely | Incomplete or missing bounds checks | Missing | `INCORRECT` |
| R-07 | Oversized Transport payloads are rejected | Length is narrowed before validation | Missing | `INCORRECT` |
| R-08 | PC build and configured tests pass | Build succeeds | 5/5 CTest targets pass | `IMPLEMENTED` |
| R-09 | ARM firmware build succeeds | Not verified locally | Not run | `UNVERIFIED` |

## 4. Findings

### ISSUE-001: SPI Transport context is never initialized

- Severity: `BLOCKER`
- Confidence: `HIGH`
- Category: Correctness / Resource lifecycle
- Location: `mcu_common/transport/transport_interface.c:75-94`

`transport_init()` clears the complete `transport_t` object and assigns operation callbacks, but it never assigns `t->ctx`. The API also provides no parameter or follow-up function for binding a `spi_slave_t` instance.

The SPI send and receive callbacks cast the null context to `spi_transport_ctx_t *` and immediately dereference `st->slave`. Therefore, the first SPI send, receive, flush, or status operation can fault.

#### Recommended change

1. Make the driver context an explicit initialization parameter, for example:

   ```c
   int transport_init(transport_t *t, link_type_t type,
                      const char *name, void *driver_ctx);
   ```

2. For SPI, either store `spi_slave_t *` directly in `t->ctx` or store a caller-owned `spi_transport_ctx_t` with a documented lifetime.
3. Reject null `t`, `name`, driver context, buffers, and `rx_len` where required.
4. Make public wrappers reject missing operation callbacks rather than dereferencing them.
5. Add unit tests that initialize and exercise every Transport operation.

### ISSUE-002: Real-time control bypasses session authorization

- Severity: `CRITICAL`
- Confidence: `HIGH`
- Category: Safety / Authorization / Correctness
- Location: `mcu_common/protocol/link_manager.c:49-76`, `mcu_common/protocol/link_manager.c:89-98`

Bringing the SPI link up immediately grants control authority. Session validation only runs when `session_active` is true and the incoming `session_id` is nonzero. Consequently:

- A control frame can be accepted before any session is established.
- A frame with `session_id == 0` bypasses validation even when a session is active.
- USB or non-rescue USART can submit `MSG_SESSION_ESTABLISH` and replace the active SPI session.

This violates the stated session-management and control-authority requirements and can allow unauthenticated control or denial of service against the active SPI session.

#### Recommended change

1. Require all real-time control messages to satisfy:
   - source is `LINK_SPI`;
   - SPI link is active;
   - `session_active == true`;
   - `session_id != 0`;
   - `session_id == active_session_id`;
   - control authority is granted;
   - sequence and TTL checks pass.
2. Restrict session establishment and termination to explicitly authorized links.
3. Reject a zero session ID during session establishment.
4. Handle `MSG_SESSION_TERMINATE` explicitly and clear session state consistently.
5. Add negative tests for control before session establishment, zero session IDs, mismatched sessions, and session takeover attempts from USB and USART.

### ISSUE-003: Invalid link identifiers can access memory out of bounds

- Severity: `MAJOR`
- Confidence: `HIGH`
- Category: Memory safety / Boundary validation
- Location: `mcu_common/protocol/link_manager.c:20-27`, `mcu_common/protocol/link_manager.c:89-105`, `mcu_common/protocol/link_manager.c:145-149`

`link_manager_evaluate()` indexes `mgr->links[src]` without checking `src`. Other functions only check `link >= 4`; because an enum can hold a negative integer, a value such as `(link_type_t)-1` passes that check and indexes before the array.

#### Recommended change

Introduce one shared validator and use it before every array access:

```c
static bool link_type_is_valid(link_type_t link)
{
    return link >= LINK_SPI && link <= LINK_USART;
}
```

Add tests for `LINK_NONE`, `4`, and `(link_type_t)-1` across evaluate, link state, and statistics APIs.

### ISSUE-004: SPI integration tests bypass the Transport implementation

- Severity: `MAJOR`
- Confidence: `HIGH`
- Category: Test quality / Requirement verification
- Location: `tests/integration/test_spi_full_duplex.c:17-125`

The integration test directly exchanges arrays through `mock_spi_link_t`, calls the protocol codec, and invokes the Link Manager. It never calls `transport_init()`, `transport_send()`, `transport_recv()`, or the SPI slave driver.

The test therefore validates protocol framing and a mock buffer exchange, but it does not validate the newly introduced Transport integration. This is why ISSUE-001 remains undetected while all tests pass.

#### Recommended change

Create a Transport-level integration fixture with a real `transport_t`, a bound mock or SPI driver context, and calls through only the public Transport API. Cover:

- initialization and open/close;
- successful send and receive;
- transfer timeout;
- null and undersized receive buffers;
- oversized transmit payload;
- driver error propagation;
- repeated full-duplex exchanges.

README should describe P2 as partial until these tests and the real MCU task integration are complete.

### ISSUE-005: Transport send length is silently truncated

- Severity: `MAJOR`
- Confidence: `HIGH`
- Category: Boundary validation / Data integrity
- Location: `mcu_common/transport/transport_interface.c:11-15`

The public API accepts a `uint32_t len`, but the SPI callback converts it to `uint16_t` before passing it to the driver. A value such as `65536` becomes zero. Other oversized values can become an apparently valid smaller length.

#### Recommended change

Validate the original value before conversion:

```c
if (len > SPI_FRAME_SIZE || len > UINT16_MAX) {
    return -1;
}
```

Also define and use named Transport error codes so callers can distinguish invalid arguments, timeout, unavailable link, and driver failures.

## 5. Recommended Fix Order

1. Redesign Transport initialization so SPI context ownership and lifetime are explicit.
2. Enforce active nonzero sessions for every real-time control message.
3. Restrict session lifecycle messages to authorized links.
4. Add shared link-type validation before all array indexing.
5. Validate payload length before narrowing conversions.
6. Replace the current SPI integration test with a Transport API end-to-end test.
7. Update README completion claims only after PC and ARM verification passes.

## 6. Checks Run

| Check | Result | Evidence |
|---|---|---|
| Fetch latest `origin/main` | `PASS` | Remote target resolved to `b61b539` |
| Fast-forward `codex` to `origin/main` | `PASS` | `dd63498..b61b539` |
| PC CMake configure | `PASS` | Visual Studio 2022 generator completed |
| PC Debug build | `PASS` | All selected targets built |
| CTest | `PASS` | 5/5 configured test executables passed |
| ARM toolchain check | `SKIP` | `arm-none-eabi-gcc` is unavailable |
| ARM firmware build | `UNVERIFIED` | Required compiler unavailable |

MSVC emitted multiple `C4819` source-encoding warnings. They did not fail this build but should be resolved to keep comments and documentation stable across toolchains.

## 7. Residual Risk

- The firmware was not built with the ARM toolchain.
- No hardware-in-loop SPI, USB, or USART verification was available.
- Heartbeat timeout, error threshold, degraded-link behavior, and failover decisions remain unimplemented or untested in the reviewed Link Manager.
- The current review does not cover the separate uncommitted safety-state-machine work in `mcu_a/application/app_tasks.c`.

## 8. Final Recommendation

Keep the review status at `REQUEST_CHANGES`. The Transport null-context defect and session-authorization bypass must be fixed before the P2 implementation is considered complete. After fixes, rerun the PC build and tests, perform an ARM build, and add Transport-level and session-negative-path tests before verification.
