/* 协议编解码单元测试 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "protocol_codec.h"
#include "protocol_crc.h"

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) do { tests_run++; printf("  TEST %s ... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("FAIL: %s\n", msg); } while(0)
#define ASSERT_EQ(a,b) if((a)!=(b)){FAIL("assertion failed");return;}
#define ASSERT_OK(r) if((r)!=PROTO_OK){FAIL("expected PROTO_OK");return;}

/* ---- Header 序列化往返测试 ---- */
static void test_header_roundtrip(void) {
    TEST("header roundtrip");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_REALTIME_CONTROL, NODE_ID_RK, NODE_ID_MCU_A, MCU_ROLE_RK);
    h.session_id = 0x12345678;
    h.sequence_number = 100;
    h.timestamp_ms = 0xDEADBEEF;
    h.cmd_ttl_ms = 50;
    h.payload_length = 64;

    uint8_t buf[32];
    int ret = protocol_header_serialize(&h, buf, sizeof(buf));
    ASSERT_OK(ret);

    ProtocolHeader h2;
    ret = protocol_header_deserialize(buf, sizeof(buf), &h2);
    ASSERT_OK(ret);

    ASSERT_EQ(h.sync, h2.sync);
    ASSERT_EQ(h.session_id, h2.session_id);
    ASSERT_EQ(h.sequence_number, h2.sequence_number);
    ASSERT_EQ(h.timestamp_ms, h2.timestamp_ms);
    ASSERT_EQ(h.cmd_ttl_ms, h2.cmd_ttl_ms);
    ASSERT_EQ(h.payload_length, h2.payload_length);
    ASSERT_EQ(h.mcu_role, h2.mcu_role);
    PASS();
}

/* ---- 帧序列化往返测试 ---- */
static void test_frame_roundtrip(void) {
    TEST("frame roundtrip");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_FAULT_EVENT, NODE_ID_MCU_A, NODE_ID_RK, MCU_ROLE_CHASSIS);
    h.session_id = 0xAABBCCDD;
    h.sequence_number = 42;

    const char *payload_str = "FAULT: Bus Off ch0";
    uint8_t payload[64];
    uint16_t plen = (uint16_t)strlen(payload_str) + 1;
    memcpy(payload, payload_str, plen);

    uint8_t buf[512];
    size_t out_len;
    int ret = protocol_frame_serialize(&h, payload, plen, buf, sizeof(buf), &out_len);
    ASSERT_OK(ret);
    ASSERT_EQ((int)out_len, PROTOCOL_HEADER_LENGTH + plen + 2);

    ProtocolHeader h2;
    const uint8_t *rx_payload;
    uint16_t rx_plen;
    ret = protocol_frame_deserialize(buf, out_len, &h2, &rx_payload, &rx_plen);
    ASSERT_OK(ret);

    ASSERT_EQ(h.session_id, h2.session_id);
    ASSERT_EQ(h.sequence_number, h2.sequence_number);
    ASSERT_EQ(plen, rx_plen);
    ASSERT_OK(memcmp(payload, rx_payload, plen));
    PASS();
}

/* ---- 同步字搜索测试 ---- */
static void test_sync_find(void) {
    TEST("sync find");
    uint8_t buf[100];
    memset(buf, 0xFF, sizeof(buf));
    buf[50] = 0x5A; buf[51] = 0xA5;
    int offset = protocol_find_sync(buf, sizeof(buf));
    ASSERT_EQ(offset, 50);
    PASS();
}

static void test_sync_not_found(void) {
    TEST("sync not found");
    uint8_t buf[10];
    memset(buf, 0xFF, sizeof(buf));
    int offset = protocol_find_sync(buf, sizeof(buf));
    ASSERT_EQ(offset, -1);
    PASS();
}

/* ---- CRC 错误检测 ---- */
static void test_crc_error_detection(void) {
    TEST("CRC error detection");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_HEARTBEAT, NODE_ID_MCU_A, NODE_ID_RK, MCU_ROLE_CHASSIS);

    uint8_t buf[32];
    protocol_header_serialize(&h, buf, sizeof(buf));

    /* 破坏 CRC */
    buf[31] ^= 0xFF;

    ProtocolHeader h2;
    int ret = protocol_header_deserialize(buf, sizeof(buf), &h2);
    ASSERT_EQ(ret, PROTO_ERR_CRC_HEADER);
    PASS();
}

/* ---- 版本不匹配 ---- */
static void test_version_mismatch(void) {
    TEST("version mismatch");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_HEARTBEAT, NODE_ID_RK, NODE_ID_MCU_A, MCU_ROLE_RK);
    uint8_t buf[32];
    protocol_header_serialize(&h, buf, sizeof(buf));
    buf[2] = 99; /* 篡改版本号 — 需要同时修正 CRC */
    buf[31] = crc8_compute(buf, 31); /* 重新计算 CRC */

    ProtocolHeader h2;
    int ret = protocol_header_deserialize(buf, sizeof(buf), &h2);
    ASSERT_EQ(ret, PROTO_ERR_INVALID_VERSION);
    PASS();
}

/* ---- 控制命令校验 ---- */
static void test_command_validation_ok(void) {
    TEST("command validation OK");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_REALTIME_CONTROL, NODE_ID_RK, NODE_ID_MCU_A, MCU_ROLE_RK);
    h.session_id = 0xBEEF;
    h.sequence_number = 200;
    h.timestamp_ms = 10000;
    h.cmd_ttl_ms = 50;

    int ret = protocol_validate_command(&h, 199, 9000, 10020, 0xBEEF);
    ASSERT_OK(ret);
    PASS();
}

static void test_command_validation_outdated_seq(void) {
    TEST("command validation - outdated seq");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_REALTIME_CONTROL, NODE_ID_RK, NODE_ID_MCU_A, MCU_ROLE_RK);
    h.session_id = 0xBEEF;
    h.sequence_number = 100; /* < last_accepted=199 */

    int ret = protocol_validate_command(&h, 199, 9000, 10000, 0xBEEF);
    ASSERT_EQ(ret, PROTO_ERR_SEQ_OUTDATED);
    PASS();
}

static void test_command_validation_expired(void) {
    TEST("command validation - TTL expired");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_REALTIME_CONTROL, NODE_ID_RK, NODE_ID_MCU_A, MCU_ROLE_RK);
    h.session_id = 0xBEEF;
    h.sequence_number = 200;
    h.timestamp_ms = 10000;
    h.cmd_ttl_ms = 10; /* 10ms TTL */

    /* 当前时间 10030ms, 已过 30ms > TTL 10ms */
    int ret = protocol_validate_command(&h, 199, 9000, 10030, 0xBEEF);
    ASSERT_EQ(ret, PROTO_ERR_TTL_EXPIRED);
    PASS();
}

static void test_command_validation_bad_session(void) {
    TEST("command validation - bad session");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_REALTIME_CONTROL, NODE_ID_RK, NODE_ID_MCU_A, MCU_ROLE_RK);
    h.session_id = 0xDEAD;
    h.sequence_number = 200;
    h.timestamp_ms = 10000;

    int ret = protocol_validate_command(&h, 199, 9000, 10000, 0xBEEF); /* different session */
    ASSERT_EQ(ret, PROTO_ERR_SESSION_INVALID);
    PASS();
}

/* ---- 边界测试 ---- */
static void test_empty_payload(void) {
    TEST("empty payload");
    ProtocolHeader h;
    protocol_header_init(&h, MSG_HEARTBEAT, NODE_ID_MCU_A, NODE_ID_RK, MCU_ROLE_CHASSIS);

    uint8_t buf[256];
    size_t out_len;
    int ret = protocol_frame_serialize(&h, NULL, 0, buf, sizeof(buf), &out_len);
    ASSERT_OK(ret);
    ASSERT_EQ((int)out_len, PROTOCOL_HEADER_LENGTH + 2); /* header + CRC16 only */

    ProtocolHeader h2;
    const uint8_t *payload;
    uint16_t plen;
    ret = protocol_frame_deserialize(buf, out_len, &h2, &payload, &plen);
    ASSERT_OK(ret);
    ASSERT_EQ((int)plen, 0);
    PASS();
}

static void test_msg_type_names(void) {
    TEST("message type names");
    ASSERT_EQ(strcmp(protocol_msg_type_name(MSG_HEARTBEAT), "HEARTBEAT"), 0);
    ASSERT_EQ(strcmp(protocol_msg_type_name(MSG_REALTIME_CONTROL), "REALTIME_CONTROL"), 0);
    ASSERT_EQ(strcmp(protocol_msg_type_name(MSG_SAFETY_NOTIFY), "SAFETY_NOTIFY"), 0);
    ASSERT_EQ(strcmp(protocol_msg_type_name(0xFF), "UNKNOWN"), 0);
    PASS();
}

/* ---- 运行所有测试 ---- */
int main(void) {
    printf("\n=== Protocol Codec Tests ===\n\n");

    test_header_roundtrip();
    test_frame_roundtrip();
    test_sync_find();
    test_sync_not_found();
    test_crc_error_detection();
    test_version_mismatch();
    test_command_validation_ok();
    test_command_validation_outdated_seq();
    test_command_validation_expired();
    test_command_validation_bad_session();
    test_empty_payload();
    test_msg_type_names();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
