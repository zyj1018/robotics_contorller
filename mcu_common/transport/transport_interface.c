#include "transport_interface.h"
#include <string.h>
#include "../drivers/spi_slave_driver.h"

/* ---- SPI Slave Transport ---- */
typedef struct { spi_slave_t *slave; } spi_transport_ctx_t;

static int spi_transport_open(void *ctx) { (void)ctx; return 0; }
static int spi_transport_close(void *ctx) { (void)ctx; return 0; }

static int spi_transport_send(void *ctx, const uint8_t *data, uint32_t len, uint32_t timeout_ms) {
    spi_transport_ctx_t *st = (spi_transport_ctx_t *)ctx;
    (void)timeout_ms;
    return spi_slave_prepare_response(st->slave, data, (uint16_t)len);
}

static int spi_transport_recv(void *ctx, uint8_t *buf, uint32_t cap, uint32_t *rx_len, uint32_t timeout_ms) {
    spi_transport_ctx_t *st = (spi_transport_ctx_t *)ctx;
    if (spi_slave_wait_transfer(st->slave, timeout_ms) != 0) return -1;
    if (cap < SPI_FRAME_SIZE) return -1;
    memcpy(buf, st->slave->rx_buf, SPI_FRAME_SIZE);
    *rx_len = SPI_FRAME_SIZE;
    st->slave->rx_complete = false;
    return 0;
}

static int spi_transport_flush(void *ctx) {
    spi_transport_ctx_t *st = (spi_transport_ctx_t *)ctx;
    return spi_slave_prepare_response(st->slave, NULL, 0);
}

static int spi_transport_status(void *ctx) {
    spi_transport_ctx_t *st = (spi_transport_ctx_t *)ctx;
    return st->slave->rx_complete ? 1 : 0;
}

static transport_ops_t spi_transport_ops = {
    .open  = spi_transport_open,
    .close_link = spi_transport_close,
    .send  = spi_transport_send,
    .recv  = spi_transport_recv,
    .flush = spi_transport_flush,
    .get_status = spi_transport_status,
};

/* ---- USB CDC Transport (stub) ---- */
static int usb_transport_open(void *ctx)  { (void)ctx; return 0; }
static int usb_transport_close(void *ctx) { (void)ctx; return 0; }
static int usb_transport_send(void *ctx, const uint8_t *data, uint32_t len, uint32_t to) { (void)ctx;(void)data;(void)len;(void)to; return -1; }
static int usb_transport_recv(void *ctx, uint8_t *buf, uint32_t cap, uint32_t *rx_len, uint32_t to) { (void)ctx;(void)buf;(void)cap;(void)rx_len;(void)to; return -1; }
static int usb_transport_flush(void *ctx)  { (void)ctx; return 0; }
static int usb_transport_status(void *ctx) { (void)ctx; return 0; }

static transport_ops_t usb_transport_ops = {
    .open  = usb_transport_open, .close_link = usb_transport_close,
    .send  = usb_transport_send, .recv  = usb_transport_recv,
    .flush = usb_transport_flush, .get_status = usb_transport_status,
};

/* ---- USART Transport (stub) ---- */
static int uart_transport_open(void *ctx)  { (void)ctx; return 0; }
static int uart_transport_close(void *ctx) { (void)ctx; return 0; }
static int uart_transport_send(void *ctx, const uint8_t *data, uint32_t len, uint32_t to) { (void)ctx;(void)data;(void)len;(void)to; return -1; }
static int uart_transport_recv(void *ctx, uint8_t *buf, uint32_t cap, uint32_t *rx_len, uint32_t to) { (void)ctx;(void)buf;(void)cap;(void)rx_len;(void)to; return -1; }
static int uart_transport_flush(void *ctx)  { (void)ctx; return 0; }
static int uart_transport_status(void *ctx) { (void)ctx; return 0; }

static transport_ops_t uart_transport_ops = {
    .open  = uart_transport_open, .close_link = uart_transport_close,
    .send  = uart_transport_send, .recv  = uart_transport_recv,
    .flush = uart_transport_flush, .get_status = uart_transport_status,
};

/* ---- Transport 工厂 ---- */
int transport_init(transport_t *t, link_type_t type, const char *name) {
    memset(t, 0, sizeof(*t));
    t->link_type = type;
    strncpy(t->name, name, sizeof(t->name)-1);
    switch (type) {
    case LINK_SPI:   t->ops = spi_transport_ops;  break;
    case LINK_USB:   t->ops = usb_transport_ops;  break;
    case LINK_USART: t->ops = uart_transport_ops; break;
    default: return -1;
    }
    return 0;
}

int transport_open(transport_t *t)  { return t->ops.open(t->ctx); }
int transport_close(transport_t *t) { return t->ops.close_link(t->ctx); }
int transport_send(transport_t *t, const uint8_t *data, uint32_t len, uint32_t timeout_ms) {
    return t->ops.send(t->ctx, data, len, timeout_ms);
}
int transport_recv(transport_t *t, uint8_t *buf, uint32_t cap, uint32_t *rx_len, uint32_t timeout_ms) {
    return t->ops.recv(t->ctx, buf, cap, rx_len, timeout_ms);
}
