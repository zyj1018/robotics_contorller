#include "protocol_types.h"
/* 协议会话管理单元测试 */
#include <stdio.h>
#include <string.h>
#include "protocol_session.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) do { tests_run++; printf("  TEST %s ... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("FAIL: %s\n", msg); return; } while(0)
#define ASSERT_EQ(a,b) if((a)!=(b)){FAIL("assertion failed");return;}
#define ASSERT_OK(r) if((r)!=PROTO_OK){FAIL("expected PROTO_OK");return;}

static void test_session_init(void) {
    TEST("session init");
    ProtocolSession s;
    protocol_session_init(&s, 50);
    ASSERT_EQ(s.session_active, false);
    ASSERT_EQ(s.heartbeat_timeout_ms, (uint32_t)50);
    PASS();
}

static void test_session_start_end(void) {
    TEST("session start/end");
    ProtocolSession s;
    protocol_session_init(&s, 50);

    uint32_t sid = protocol_session_generate_id();
    protocol_session_start(&s, sid, NODE_ID_RK, 1);
    ASSERT_EQ(s.session_active, true);
    ASSERT_EQ(s.session_id, sid);

    protocol_session_end(&s);
    ASSERT_EQ(s.session_active, false);
    PASS();
}

static void test_session_heartbeat(void) {
    TEST("session heartbeat alive/dead");
    ProtocolSession s;
    protocol_session_init(&s, 50);
    protocol_session_start(&s, 0x1234, NODE_ID_RK, 1);

    /* 收到心跳, 时间 1000 */
    protocol_session_heartbeat_rx(&s, 1000, 0);
    ASSERT_EQ(protocol_session_is_peer_alive(&s, 1020), true);  /* 20ms < 50ms */
    ASSERT_EQ(protocol_session_is_peer_alive(&s, 1100), false); /* 100ms > 50ms */
    PASS();
}

static void test_session_validate_ok(void) {
    TEST("session validate inbound OK");
    ProtocolSession s;
    protocol_session_init(&s, 50);
    protocol_session_start(&s, 0xBEEF, NODE_ID_RK, 1);

    int ret = protocol_session_validate_inbound(&s, 10, 5000, 100, 0xBEEF, 5050);
    ASSERT_OK(ret);
    ASSERT_EQ(s.last_accepted_seq, (uint32_t)10);
    PASS();
}

static void test_session_validate_seq_replay(void) {
    TEST("session validate - seq replay rejected");
    ProtocolSession s;
    protocol_session_init(&s, 50);
    protocol_session_start(&s, 0xBEEF, NODE_ID_RK, 1);

    /* 接受 seq=10 */
    protocol_session_validate_inbound(&s, 10, 5000, 100, 0xBEEF, 5050);
    /* 重放 seq=10 → 应拒绝 */
    int ret = protocol_session_validate_inbound(&s, 10, 5000, 100, 0xBEEF, 5060);
    ASSERT_EQ(ret, PROTO_ERR_SEQ_OUTDATED);
    PASS();
}

static void test_session_validate_expired(void) {
    TEST("session validate - TTL expired");
    ProtocolSession s;
    protocol_session_init(&s, 50);
    protocol_session_start(&s, 0xBEEF, NODE_ID_RK, 1);

    /* timestamp=5000, TTL=50, 当前时间=5100 → 100ms > 50ms */
    int ret = protocol_session_validate_inbound(&s, 20, 5000, 50, 0xBEEF, 5100);
    ASSERT_EQ(ret, PROTO_ERR_TTL_EXPIRED);
    PASS();
}

static void test_session_next_tx_seq(void) {
    TEST("session next TX sequence");
    ProtocolSession s;
    protocol_session_init(&s, 50);
    ASSERT_EQ(protocol_session_next_tx_seq(&s), (uint32_t)1);
    ASSERT_EQ(protocol_session_next_tx_seq(&s), (uint32_t)2);
    ASSERT_EQ(protocol_session_next_tx_seq(&s), (uint32_t)3);
    PASS();
}

int main(void) {
    printf("\n=== Protocol Session Tests ===\n\n");
    test_session_init();
    test_session_start_end();
    test_session_heartbeat();
    test_session_validate_ok();
    test_session_validate_seq_replay();
    test_session_validate_expired();
    test_session_next_tx_seq();
    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
