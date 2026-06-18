#include <stdio.h>
#include <string.h>
#include "link_manager.h"

static int tests_run=0, tests_passed=0, tests_failed=0;
#define TEST(n) do{tests_run++;printf("  TEST %s ... ",n);}while(0)
#define PASS() do{tests_passed++;printf("PASS\n");}while(0)
#define FAIL(m) do{tests_failed++;printf("FAIL: %s\n",m);return;}while(0)
#define ASSERT_EQ(a,b) if((a)!=(b)){FAIL("assertion failed");}

static void test_init(void) {
    TEST("LM init");
    link_manager_t m;
    link_manager_init(&m);
    ASSERT_EQ(m.active_link, LINK_NONE);
    ASSERT_EQ(m.session_active, false);
    PASS();
}

static void test_spi_up_becomes_active(void) {
    TEST("SPI up → active link");
    link_manager_t m;
    link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    ASSERT_EQ(link_manager_active_link(&m), LINK_SPI);
    /* 需要建立会话后才能获得控制权 */
    ASSERT_EQ(link_manager_can_control(&m, LINK_SPI), false);
    link_manager_session_start(&m, 0x1234);
    ASSERT_EQ(link_manager_can_control(&m, LINK_SPI), true);
    PASS();
}

static void test_usb_rejects_control(void) {
    TEST("USB rejects control command");
    link_manager_t m;
    link_manager_init(&m);
    link_manager_link_up(&m, LINK_USB);
    link_decision_t d = link_manager_evaluate(&m, LINK_USB, MSG_REALTIME_CONTROL,
                                               0, 1, 1000, 50, 1020);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    PASS();
}

static void test_spi_accepts_control(void) {
    TEST("SPI accepts control command");
    link_manager_t m;
    link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    link_manager_session_start(&m, 0x1234);
    link_decision_t d = link_manager_evaluate(&m, LINK_SPI, MSG_REALTIME_CONTROL,
                                               0x1234, 10, 1000, 50, 1020);
    ASSERT_EQ(d, LINK_DECISION_ACCEPT);
    ASSERT_EQ(m.last_accepted_seq, (uint32_t)10);
    PASS();
}

static void test_seq_replay_rejected(void) {
    TEST("seq replay rejected");
    link_manager_t m;
    link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    link_manager_session_start(&m, 0xBEEF);
    link_manager_evaluate(&m, LINK_SPI, MSG_REALTIME_CONTROL, 0xBEEF, 10, 1000, 50, 1020);
    link_decision_t d = link_manager_evaluate(&m, LINK_SPI, MSG_REALTIME_CONTROL, 0xBEEF, 10, 1000, 50, 1030);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    PASS();
}

static void test_ttl_expired_rejected(void) {
    TEST("TTL expired rejected");
    link_manager_t m;
    link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    link_manager_session_start(&m, 0xBEEF);
    link_decision_t d = link_manager_evaluate(&m, LINK_SPI, MSG_REALTIME_CONTROL,
                                               0xBEEF, 20, 1000, 10, 1050);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    PASS();
}

static void test_spi_down_revokes_control(void) {
    TEST("SPI down revokes control");
    link_manager_t m;
    link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    link_manager_link_down(&m, LINK_SPI);
    ASSERT_EQ(link_manager_can_control(&m, LINK_SPI), false);
    ASSERT_EQ(m.session_active, false);
    PASS();
}

static void test_usart_rescue_limits(void) {
    TEST("USART rescue mode limits commands");
    link_manager_t m;
    link_manager_init(&m);
    link_manager_link_up(&m, LINK_USART);
    m.links[LINK_USART].rescue_mode = true;

    /* 固件升级 -> 允许 */
    link_decision_t d1 = link_manager_evaluate(&m, LINK_USART, MSG_FIRMWARE_UPGRADE,
                                                0, 0, 0, 0, 0);
    ASSERT_EQ(d1, LINK_DECISION_ACCEPT);

    /* 实时控制 -> 拒绝 */
    link_decision_t d2 = link_manager_evaluate(&m, LINK_USART, MSG_REALTIME_CONTROL,
                                                0, 1, 1000, 50, 1020);
    ASSERT_EQ(d2, LINK_DECISION_REJECT);
    PASS();
}


/* ---- 新增测试: 会话强制 + 边界 ---- */

static void test_reject_control_before_session(void) {
    TEST("reject control before session");
    link_manager_t m; link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    /* SPI已UP但无会话 → 拒绝控制 */
    link_decision_t d = link_manager_evaluate(&m, LINK_SPI, MSG_REALTIME_CONTROL,
                                               0x1234, 1, 1000, 50, 1020);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    PASS();
}

static void test_reject_zero_session_id_control(void) {
    TEST("reject session_id=0 during session");
    link_manager_t m; link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    /* 先建立会话再发起session_id=0的控制 */
    link_manager_evaluate(&m, LINK_SPI, MSG_SESSION_ESTABLISH, 0xBEEF, 0, 0, 0, 0);
    link_decision_t d = link_manager_evaluate(&m, LINK_SPI, MSG_REALTIME_CONTROL,
                                               0, 10, 1000, 50, 1020);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    PASS();
}

static void test_reject_session_mismatch(void) {
    TEST("reject mismatched session");
    link_manager_t m; link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    link_manager_evaluate(&m, LINK_SPI, MSG_SESSION_ESTABLISH, 0x1234, 0, 0, 0, 0);
    link_decision_t d = link_manager_evaluate(&m, LINK_SPI, MSG_REALTIME_CONTROL,
                                               0x5678, 10, 1000, 50, 1020);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    PASS();
}

static void test_reject_usb_session_establish(void) {
    TEST("USB cannot establish session");
    link_manager_t m; link_manager_init(&m);
    link_manager_link_up(&m, LINK_USB);
    link_decision_t d = link_manager_evaluate(&m, LINK_USB, MSG_SESSION_ESTABLISH,
                                               0xBEEF, 0, 0, 0, 0);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    ASSERT_EQ(m.session_active, false);
    PASS();
}

static void test_reject_zero_session_establish(void) {
    TEST("reject session_id=0 establish");
    link_manager_t m; link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    link_decision_t d = link_manager_evaluate(&m, LINK_SPI, MSG_SESSION_ESTABLISH,
                                               0, 0, 0, 0, 0);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    PASS();
}

static void test_invalid_link_type_rejected(void) {
    TEST("invalid link type rejected");
    link_manager_t m; link_manager_init(&m);
    link_decision_t d = link_manager_evaluate(&m, (link_type_t)4, MSG_HEARTBEAT,
                                               0, 0, 0, 0, 0);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    d = link_manager_evaluate(&m, (link_type_t)-1, MSG_HEARTBEAT, 0, 0, 0, 0, 0);
    ASSERT_EQ(d, LINK_DECISION_REJECT);
    PASS();
}

static void test_session_terminate_ok(void) {
    TEST("session terminate via SPI");
    link_manager_t m; link_manager_init(&m);
    link_manager_link_up(&m, LINK_SPI);
    link_manager_evaluate(&m, LINK_SPI, MSG_SESSION_ESTABLISH, 0xBEEF, 0, 0, 0, 0);
    ASSERT_EQ(m.session_active, true);
    link_decision_t d = link_manager_evaluate(&m, LINK_SPI, MSG_SESSION_TERMINATE,
                                               0xBEEF, 0, 0, 0, 0);
    ASSERT_EQ(d, LINK_DECISION_ACCEPT);
    ASSERT_EQ(m.session_active, false);
    PASS();
}

int main(void) {
    printf("\n=== Link Manager Tests ===\n\n");
    test_init();
    test_spi_up_becomes_active();
    test_usb_rejects_control();
    test_spi_accepts_control();
    test_seq_replay_rejected();
    test_ttl_expired_rejected();
    test_spi_down_revokes_control();
    test_usart_rescue_limits();
    test_reject_control_before_session();
    test_reject_zero_session_id_control();
    test_reject_session_mismatch();
    test_reject_usb_session_establish();
    test_reject_zero_session_establish();
    test_invalid_link_type_rejected();
    test_session_terminate_ok();
    printf("\n=== Results: %d run, %d passed, %d failed ===\n", tests_run, tests_passed, tests_failed);
    return tests_failed>0?1:0;
}

