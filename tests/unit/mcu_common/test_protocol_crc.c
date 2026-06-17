/* CRC 校验单元测试 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "protocol_crc.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) do { tests_run++; printf("  TEST %s ... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("FAIL: %s\n", msg); return; } while(0)
#define ASSERT_EQ(a,b) if((a)!=(b)){FAIL("assertion failed");return;}

/* 已知 CRC8 测试向量 */
static void test_crc8_known_vector(void) {
    TEST("CRC8 known vector");
    /* "123456789" → CRC-8/Dallas 应为 0x04 */
    const uint8_t data[] = "123456789";
    uint8_t crc = crc8_compute(data, 9);
    ASSERT_EQ(crc, 0x04);
    PASS();
}

static void test_crc8_empty(void) {
    TEST("CRC8 empty data");
    uint8_t crc = crc8_compute((const uint8_t *)"", 0);
    /* CRC8 of empty: initial 0xFF, final XOR 0xFF → 0x00 */
    ASSERT_EQ(crc, 0x00);
    PASS();
}

/* 已知 CRC16-CCITT 测试向量 */
static void test_crc16_known_vector(void) {
    TEST("CRC16-CCITT known vector");
    /* "123456789" → CRC-16-CCITT 应为 0x29B1 */
    const uint8_t data[] = "123456789";
    uint16_t crc = crc16_compute(data, 9);
    ASSERT_EQ(crc, 0x29B1);
    PASS();
}

static void test_crc16_empty(void) {
    TEST("CRC16 empty data");
    uint16_t crc = crc16_compute((const uint8_t *)"", 0);
    /* CRC16 of empty: initial 0xFFFF → 0xFFFF (no data processed, just initial) */
    ASSERT_EQ(crc, 0xFFFF);
    PASS();
}

/* CRC一致性测试 */
static void test_crc_consistency(void) {
    TEST("CRC consistency (same input → same output)");
    const char *str = "Hello Robot Controller Protocol Test!";
    size_t len = strlen(str);
    uint16_t crc1 = crc16_compute((const uint8_t *)str, len);
    uint16_t crc2 = crc16_compute((const uint8_t *)str, len);
    ASSERT_EQ(crc1, crc2);
    PASS();
}

static void test_crc_difference(void) {
    TEST("CRC difference (different input → different output)");
    const char *a = "MotionCommand:100";
    const char *b = "MotionCommand:101";
    uint16_t ca = crc16_compute((const uint8_t *)a, strlen(a));
    uint16_t cb = crc16_compute((const uint8_t *)b, strlen(b));
    if((ca)==(cb)){FAIL("should differ");return;}
    PASS();
}

int main(void) {
    printf("\n=== Protocol CRC Tests ===\n\n");
    test_crc8_known_vector();
    test_crc8_empty();
    test_crc16_known_vector();
    test_crc16_empty();
    test_crc_consistency();
    test_crc_difference();
    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
