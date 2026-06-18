/* SPI 全双工集成测试: 模拟 RK ↔ MCU 帧交换 */
#include <stdio.h>
#include <string.h>
#include "protocol_frame.h"
#include "protocol_codec.h"
#include "protocol_session.h"
#include "link_manager.h"
#include "mock_spi.h"

static int tests_run=0,tests_passed=0,tests_failed=0;
#define TEST(n) do{tests_run++;printf("  TEST %s ... ",n);}while(0)
#define PASS() do{tests_passed++;printf("PASS\n");}while(0)
#define FAIL(m) do{tests_failed++;printf("FAIL: %s\n",m);return;}while(0)
#define ASSERT_EQ(a,b) if((a)!=(b)){FAIL("assertion failed");}
#define ASSERT_OK(r) if((r)!=0){FAIL("expected OK");}

static void test_spi_control_roundtrip(void) {
    TEST("SPI control round-trip");
    mock_spi_link_t link; mock_spi_link_init(&link);
    link_manager_t lm; link_manager_init(&lm);
    link_manager_link_up(&lm, LINK_SPI); link_manager_session_start(&lm, 0xCAFE0001);

    /* MCU preload idle frame → slave_tx */
    ProtocolHeader ih; size_t il;
    protocol_header_init(&ih, MSG_IDLE_FRAME, NODE_ID_MCU_A, NODE_ID_RK, MCU_ROLE_CHASSIS);
    protocol_frame_serialize(&ih, NULL, 0, link.slave_tx, MOCK_SPI_BUF_SIZE, &il);
    memset(link.slave_tx+il, 0, MOCK_SPI_BUF_SIZE-il);

    /* RK builds control frame → master_tx */
    ProtocolHeader ch;
    protocol_header_init(&ch, MSG_REALTIME_CONTROL, NODE_ID_RK, NODE_ID_MCU_A, MCU_ROLE_RK);
    ch.session_id=0xCAFE0001; ch.sequence_number=1; ch.timestamp_ms=1000; ch.cmd_ttl_ms=50;
    float cmd[6]={1.5f,0,0,2.0f,0.5f,0.3f};
    ASSERT_OK(protocol_frame_serialize(&ch,(uint8_t*)cmd,(uint16_t)sizeof(cmd),link.master_tx,MOCK_SPI_BUF_SIZE,&il));
    memset(link.master_tx+il,0,MOCK_SPI_BUF_SIZE-il);

    /* SPI exchange */
    mock_spi_master_transfer(&link);

    /* MCU decodes RK frame */
    ProtocolHeader rx; const uint8_t *rp; uint16_t rpl;
    ASSERT_OK(protocol_frame_deserialize(link.slave_rx,MOCK_SPI_BUF_SIZE,&rx,&rp,&rpl));
    ASSERT_EQ(rx.msg_type,(uint8_t)MSG_REALTIME_CONTROL);

    /* Link Manager validates */
    link_decision_t d=link_manager_evaluate(&lm,LINK_SPI,rx.msg_type,rx.session_id,rx.sequence_number,rx.timestamp_ms,rx.cmd_ttl_ms,1020);
    ASSERT_EQ(d,LINK_DECISION_ACCEPT);

    /* MCU builds response */
    ProtocolHeader rh;
    protocol_header_init(&rh,MSG_REALTIME_STATE,NODE_ID_MCU_A,NODE_ID_RK,MCU_ROLE_CHASSIS);
    rh.session_id=rx.session_id; rh.sequence_number=rx.sequence_number;
    float st[4]={1.48f,0.01f,0.5f,35.0f};
    protocol_frame_serialize(&rh,(uint8_t*)st,(uint16_t)sizeof(st),link.slave_tx,MOCK_SPI_BUF_SIZE,&il);
    memset(link.slave_tx+il,0,MOCK_SPI_BUF_SIZE-il);

    /* Second exchange: RK reads response */
    memset(link.master_tx,0,MOCK_SPI_BUF_SIZE);
    mock_spi_master_transfer(&link);
    ASSERT_OK(protocol_frame_deserialize(link.master_rx,MOCK_SPI_BUF_SIZE,&rx,&rp,&rpl));
    ASSERT_EQ(rx.msg_type,(uint8_t)MSG_REALTIME_STATE);
    PASS();
}

static void test_spi_lm_rejects_expired(void) {
    TEST("LM rejects expired frame");
    mock_spi_link_t link; mock_spi_link_init(&link);
    link_manager_t lm; link_manager_init(&lm);
    link_manager_link_up(&lm,LINK_SPI); link_manager_session_start(&lm,0xCAFE0001);

    ProtocolHeader ih; size_t il;
    protocol_header_init(&ih,MSG_IDLE_FRAME,NODE_ID_MCU_A,NODE_ID_RK,MCU_ROLE_CHASSIS);
    protocol_frame_serialize(&ih,NULL,0,link.slave_tx,MOCK_SPI_BUF_SIZE,&il);
    memset(link.slave_tx+il,0,MOCK_SPI_BUF_SIZE-il);

    ProtocolHeader ch;
    protocol_header_init(&ch,MSG_REALTIME_CONTROL,NODE_ID_RK,NODE_ID_MCU_A,MCU_ROLE_RK);
    ch.session_id=0xCAFE0001; ch.sequence_number=5; ch.timestamp_ms=1000; ch.cmd_ttl_ms=(uint16_t)10;
    protocol_frame_serialize(&ch,NULL,0,link.master_tx,MOCK_SPI_BUF_SIZE,&il);
    memset(link.master_tx+il,0,MOCK_SPI_BUF_SIZE-il);
    mock_spi_master_transfer(&link);

    ProtocolHeader rx; const uint8_t *rp; uint16_t rpl;
    ASSERT_OK(protocol_frame_deserialize(link.slave_rx,MOCK_SPI_BUF_SIZE,&rx,&rp,&rpl));
    link_decision_t d=link_manager_evaluate(&lm,LINK_SPI,rx.msg_type,rx.session_id,rx.sequence_number,rx.timestamp_ms,rx.cmd_ttl_ms,1100);
    ASSERT_EQ(d,LINK_DECISION_REJECT);
    PASS();
}

static void test_spi_10frame_exchange(void) {
    TEST("SPI 10-frame exchange");
    mock_spi_link_t link; mock_spi_link_init(&link);
    link_manager_t lm; link_manager_init(&lm);
    link_manager_link_up(&lm,LINK_SPI); link_manager_session_start(&lm,0xCAFE0001);

    for(int i=1;i<=10;i++){
        ProtocolHeader ih; size_t il;
        protocol_header_init(&ih,MSG_IDLE_FRAME,NODE_ID_MCU_A,NODE_ID_RK,MCU_ROLE_CHASSIS);
        protocol_frame_serialize(&ih,NULL,0,link.slave_tx,MOCK_SPI_BUF_SIZE,&il);
        memset(link.slave_tx+il,0,MOCK_SPI_BUF_SIZE-il);

        ProtocolHeader ch;
        protocol_header_init(&ch,MSG_REALTIME_CONTROL,NODE_ID_RK,NODE_ID_MCU_A,MCU_ROLE_RK);
        ch.session_id=0xCAFE0001; ch.sequence_number=(uint32_t)i; ch.timestamp_ms=(uint32_t)(1000+i); ch.cmd_ttl_ms=50;
        float cmd[6]={1.0f,0,0,2.0f,0.5f,0.3f};
        protocol_frame_serialize(&ch,(uint8_t*)cmd,(uint16_t)sizeof(cmd),link.master_tx,MOCK_SPI_BUF_SIZE,&il);
        memset(link.master_tx+il,0,MOCK_SPI_BUF_SIZE-il);
        mock_spi_master_transfer(&link);

        ProtocolHeader rx; const uint8_t *rp; uint16_t rpl;
        ASSERT_OK(protocol_frame_deserialize(link.slave_rx,MOCK_SPI_BUF_SIZE,&rx,&rp,&rpl));
        link_decision_t d=link_manager_evaluate(&lm,LINK_SPI,rx.msg_type,rx.session_id,rx.sequence_number,rx.timestamp_ms,rx.cmd_ttl_ms,(uint32_t)(1010+i));
        ASSERT_EQ(d,LINK_DECISION_ACCEPT);

        ProtocolHeader rh;
        protocol_header_init(&rh,MSG_REALTIME_STATE,NODE_ID_MCU_A,NODE_ID_RK,MCU_ROLE_CHASSIS);
        rh.session_id=rx.session_id; rh.sequence_number=rx.sequence_number;
        float st[4]={1.48f,0.01f,0.5f,35.0f};
        protocol_frame_serialize(&rh,(uint8_t*)st,(uint16_t)sizeof(st),link.slave_tx,MOCK_SPI_BUF_SIZE,&il);
        memset(link.slave_tx+il,0,MOCK_SPI_BUF_SIZE-il);
        memset(link.master_tx,0,MOCK_SPI_BUF_SIZE);
        mock_spi_master_transfer(&link);
        ASSERT_OK(protocol_frame_deserialize(link.master_rx,MOCK_SPI_BUF_SIZE,&rx,&rp,&rpl));
        ASSERT_EQ(rx.msg_type,(uint8_t)MSG_REALTIME_STATE);
    }
    PASS();
}

int main(void){
    printf("\n=== SPI Full-Duplex Integration Tests ===\n\n");
    test_spi_control_roundtrip();
    test_spi_lm_rejects_expired();
    test_spi_10frame_exchange();
    printf("\n=== Results: %d run, %d passed, %d failed ===\n",tests_run,tests_passed,tests_failed);
    return tests_failed>0?1:0;
}
