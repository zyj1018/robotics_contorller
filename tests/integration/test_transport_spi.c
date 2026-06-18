#include <stdio.h>
#include <string.h>
#include "spi_slave_driver.h"
#include "transport_interface.h"
#include "protocol_frame.h"
#include "protocol_codec.h"
#include "link_manager.h"
#include "mock_spi.h"

static int tr=0,tp=0,tf=0;
#define TEST(n) do{tr++;printf("  TEST %s ... ",n);}while(0)
#define PASS() do{tp++;printf("PASS\n");}while(0)
#define FAIL(m) do{tf++;printf("FAIL: %s\n",m);return;}while(0)
#define ASSERT_EQ(a,b) if((a)!=(b)){FAIL("assertion failed");}
#define ASSERT_OK(r) if((r)!=TRANS_OK){FAIL("expected TRANS_OK");}
#define ASSERT_ERR(r,e) if((r)!=(e)){FAIL("expected error code");}

/* 辅助: 模拟 SPI 传输完成后, 将 mock 数据同步到 slave 驱动缓冲区 */
static void mock_xfer_to_slave(mock_spi_link_t *link, spi_slave_t *slave) {
    memcpy(slave->rx_buf, link->slave_rx, SPI_FRAME_SIZE);
    slave->rx_complete = true;
}
static void slave_to_mock_xfer(spi_slave_t *slave, mock_spi_link_t *link) {
    memcpy(link->slave_tx, slave->tx_buf, SPI_FRAME_SIZE);
}

static void test_init_ok(void){
    TEST("init OK");
    spi_slave_t s; memset(&s,0,sizeof(s));
    spi_transport_ctx_t ctx={.slave=&s};
    transport_t t;
    ASSERT_EQ(transport_init(&t,LINK_SPI,"spi",&ctx),TRANS_OK);
    PASS();
}
static void test_init_null(void){
    TEST("init NULL rejected");
    transport_t t; spi_slave_t s;
    spi_transport_ctx_t ctx={.slave=&s};
    ASSERT_ERR(transport_init(NULL,LINK_SPI,"x",&ctx),TRANS_ERR_PARAM);
    ASSERT_ERR(transport_init(&t,LINK_SPI,NULL,&ctx),TRANS_ERR_PARAM);
    ASSERT_ERR(transport_init(&t,LINK_SPI,"x",NULL),TRANS_ERR_PARAM);
    PASS();
}
static void test_oversized(void){
    TEST("send oversized rejected");
    spi_slave_t s; memset(&s,0,sizeof(s));
    spi_transport_ctx_t ctx={.slave=&s};
    transport_t t;
    ASSERT_OK(transport_init(&t,LINK_SPI,"t",&ctx));
    uint8_t big[SPI_FRAME_SIZE+1]; (void)big;
    ASSERT_ERR(transport_send(&t,big,sizeof(big),0),TRANS_ERR_OVERFLOW);
    PASS();
}
static void test_roundtrip(void){
    TEST("SPI round-trip via Transport API");
    mock_spi_link_t link; mock_spi_link_init(&link);
    spi_slave_t slave; memset(&slave,0,sizeof(slave));
    spi_transport_ctx_t ctx={.slave=&slave};
    transport_t t;
    ASSERT_OK(transport_init(&t,LINK_SPI,"spi",&ctx));

    /* MCU preloads slave TX */
    ProtocolHeader ih; size_t il;
    protocol_header_init(&ih,MSG_IDLE_FRAME,NODE_ID_MCU_A,NODE_ID_RK,MCU_ROLE_CHASSIS);
    protocol_frame_serialize(&ih,NULL,0,link.slave_tx,MOCK_SPI_BUF_SIZE,&il);
    memset(link.slave_tx+il,0,MOCK_SPI_BUF_SIZE-il);

    /* RK: write control frame → master_tx, SPI exchange */
    ProtocolHeader ch;
    protocol_header_init(&ch,MSG_REALTIME_CONTROL,NODE_ID_RK,NODE_ID_MCU_A,MCU_ROLE_RK);
    ch.session_id=0xCAFE;ch.sequence_number=1;ch.timestamp_ms=1000;ch.cmd_ttl_ms=50;
    float cmd[6]={1.0f,0,0,2.0f,0.5f,0.3f};
    protocol_frame_serialize(&ch,(uint8_t*)cmd,(uint16_t)sizeof(cmd),link.master_tx,MOCK_SPI_BUF_SIZE,&il);
    memset(link.master_tx+il,0,MOCK_SPI_BUF_SIZE-il);

    /* SPI 硬件传输 */
    mock_spi_master_transfer(&link);
    /* 同步到 slave 驱动 */
    mock_xfer_to_slave(&link, &slave);

    /* MCU: Transport API 接收 */
    uint8_t rx[SPI_FRAME_SIZE]; uint32_t rxl;
    ASSERT_OK(transport_recv(&t,rx,sizeof(rx),&rxl,0));
    ASSERT_EQ((int)rxl,SPI_FRAME_SIZE);

    /* 解码验证 */
    ProtocolHeader rx_hdr;const uint8_t*rp;uint16_t rpl;
    ASSERT_OK(protocol_frame_deserialize(rx,rxl,&rx_hdr,&rp,&rpl));
    ASSERT_EQ(rx_hdr.msg_type,(uint8_t)MSG_REALTIME_CONTROL);

    /* MCU: Transport API 发送响应 */
    ProtocolHeader rh;
    protocol_header_init(&rh,MSG_REALTIME_STATE,NODE_ID_MCU_A,NODE_ID_RK,MCU_ROLE_CHASSIS);
    rh.session_id=rx_hdr.session_id;rh.sequence_number=rx_hdr.sequence_number;
    float st[4]={1.48f,0.01f,0.5f,35.0f};
    uint8_t tx[SPI_FRAME_SIZE];size_t txl;
    protocol_frame_serialize(&rh,(uint8_t*)st,(uint16_t)sizeof(st),tx,SPI_FRAME_SIZE,&txl);
    memset(tx+txl,0,SPI_FRAME_SIZE-txl);
    ASSERT_OK(transport_send(&t,tx,SPI_FRAME_SIZE,0));
    /* 同步 slave TX → mock_link (模拟 DMA 发送) */
    slave_to_mock_xfer(&slave, &link);

    /* 验证 mock_link 收到了正确的数据 */
    ASSERT_EQ(memcmp(link.slave_tx,tx,SPI_FRAME_SIZE),0);
    PASS();
}

int main(void){
    printf("\n=== Transport API Integration Tests ===\n\n");
    test_init_ok();test_init_null();test_oversized();test_roundtrip();
    printf("\n=== Results: %d run, %d passed, %d failed ===\n",tr,tp,tf);
    return tf>0?1:0;
}
