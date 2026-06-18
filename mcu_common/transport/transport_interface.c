#include "transport_interface.h"
#include <string.h>
#include "../drivers/spi_slave_driver.h"

/* ---- SPI Slave Transport ---- */
static int spi_transport_send(void *ctx, const uint8_t *data, uint32_t len, uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!ctx || !data) return TRANS_ERR_PARAM;
    if (len > SPI_FRAME_SIZE) return TRANS_ERR_OVERFLOW;
    spi_transport_ctx_t *st = (spi_transport_ctx_t *)ctx;
    return spi_slave_prepare_response(st->slave, data, (uint16_t)len);
}

static int spi_transport_recv(void *ctx, uint8_t *buf, uint32_t cap, uint32_t *rx_len, uint32_t timeout_ms) {
    if (!ctx || !buf || !rx_len) return TRANS_ERR_PARAM;
    if (cap < SPI_FRAME_SIZE) return TRANS_ERR_OVERFLOW;
    spi_transport_ctx_t *st = (spi_transport_ctx_t *)ctx;
    if (spi_slave_wait_transfer(st->slave, timeout_ms) != 0) return TRANS_ERR_TIMEOUT;
    memcpy(buf, st->slave->rx_buf, SPI_FRAME_SIZE);
    *rx_len = SPI_FRAME_SIZE;
    st->slave->rx_complete = false;
    return TRANS_OK;
}

static int spi_transport_flush(void *ctx) {
    if (!ctx) return TRANS_ERR_PARAM;
    spi_transport_ctx_t *st = (spi_transport_ctx_t *)ctx;
    return spi_slave_prepare_response(st->slave, NULL, 0);
}

static int spi_transport_open(void *ctx)  { (void)ctx; return TRANS_OK; }
static int spi_transport_close(void *ctx) { (void)ctx; return TRANS_OK; }
static int spi_transport_status(void *ctx) {
    if (!ctx) return TRANS_ERR_PARAM;
    return ((spi_transport_ctx_t*)ctx)->slave->rx_complete ? 1 : 0;
}

static transport_ops_t spi_transport_ops = {
    .open = spi_transport_open, .close_link = spi_transport_close,
    .send = spi_transport_send, .recv = spi_transport_recv,
    .flush = spi_transport_flush, .get_status = spi_transport_status,
};

/* ---- USB CDC Transport (stub) ---- */
static int stub_null(void *ctx) { (void)ctx; return TRANS_OK; }
static int stub_send(void *ctx, const uint8_t *data, uint32_t len, uint32_t to) {
    (void)ctx;(void)data;(void)len;(void)to; return TRANS_ERR_LINK;
}
static int stub_recv(void *ctx, uint8_t *b, uint32_t c, uint32_t *l, uint32_t to) {
    (void)ctx;(void)b;(void)c;(void)l;(void)to; return TRANS_ERR_LINK;
}
static transport_ops_t usb_transport_ops = {
    .open=stub_null, .close_link=stub_null, .send=stub_send,
    .recv=stub_recv, .flush=stub_null, .get_status=stub_null,
};
static transport_ops_t uart_transport_ops = {
    .open=stub_null, .close_link=stub_null, .send=stub_send,
    .recv=stub_recv, .flush=stub_null, .get_status=stub_null,
};

/* ---- Transport 工厂 (ISSUE-001: 接收 driver_ctx) ---- */
int transport_init(transport_t *t, link_type_t type, const char *name, void *driver_ctx) {
    if (!t || !name || !driver_ctx) return TRANS_ERR_PARAM;
    memset(t, 0, sizeof(*t));
    t->link_type = type;
    t->ctx = driver_ctx;
    strncpy(t->name, name, sizeof(t->name)-1);
    switch (type) {
    case LINK_SPI:   t->ops = spi_transport_ops;  break;
    case LINK_USB:   t->ops = usb_transport_ops;  break;
    case LINK_USART: t->ops = uart_transport_ops; break;
    default: return TRANS_ERR_PARAM;
    }
    return TRANS_OK;
}

int transport_open(transport_t *t)  { if(!t||!t->ops.open) return TRANS_ERR_PARAM; return t->ops.open(t->ctx); }
int transport_close(transport_t *t) { if(!t||!t->ops.close_link) return TRANS_ERR_PARAM; return t->ops.close_link(t->ctx); }
int transport_send(transport_t *t, const uint8_t *data, uint32_t len, uint32_t to) {
    if (!t || !data) return TRANS_ERR_PARAM;
    /* ISSUE-005: 长度验证 */
    if (len > SPI_FRAME_SIZE) return TRANS_ERR_OVERFLOW;
    if (!t->ops.send) return TRANS_ERR_PARAM;
    return t->ops.send(t->ctx, data, len, to);
}
int transport_recv(transport_t *t, uint8_t *buf, uint32_t cap, uint32_t *rx_len, uint32_t to) {
    if (!t || !buf || !rx_len) return TRANS_ERR_PARAM;
    if (!t->ops.recv) return TRANS_ERR_PARAM;
    return t->ops.recv(t->ctx, buf, cap, rx_len, to);
}
