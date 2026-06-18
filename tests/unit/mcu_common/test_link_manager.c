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
    printf("\n=== Results: %d run, %d passed, %d failed ===\n", tests_run, tests_passed, tests_failed);
    return tests_failed>0?1:0;
}
