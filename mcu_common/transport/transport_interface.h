#ifndef TRANSPORT_INTERFACE_H
#define TRANSPORT_INTERFACE_H
#include <stdint.h>
#include <stddef.h>
#include "link_manager.h"

/* Transport 抽象接口 (每链路独立实例) */
typedef struct transport_ops {
    int  (*open)(void *ctx);
    int  (*close_link)(void *ctx);
    int  (*send)(void *ctx, const uint8_t *data, uint32_t len, uint32_t timeout_ms);
    int  (*recv)(void *ctx, uint8_t *buf, uint32_t cap, uint32_t *rx_len, uint32_t timeout_ms);
    int  (*flush)(void *ctx);
    int  (*get_status)(void *ctx);
} transport_ops_t;

typedef struct transport {
    transport_ops_t ops;
    void           *ctx;
    link_type_t     link_type;
    uint32_t        tx_seq;
    char            name[16];
} transport_t;

/* SPI Transport 上下文 (供外部构造) */
struct spi_slave; /* forward */
typedef struct { struct spi_slave *slave; } spi_transport_ctx_t;

/* Transport 错误码 */
#define TRANS_OK           0
#define TRANS_ERR_PARAM   -1
#define TRANS_ERR_TIMEOUT -2
#define TRANS_ERR_OVERFLOW -3
#define TRANS_ERR_LINK    -4
#define TRANS_ERR_DRIVER  -5

/* Transport 工厂 */
int transport_init(transport_t *t, link_type_t type, const char *name, void *driver_ctx);
int transport_open(transport_t *t);
int transport_close(transport_t *t);
int transport_send(transport_t *t, const uint8_t *data, uint32_t len, uint32_t timeout_ms);
int transport_recv(transport_t *t, uint8_t *buf, uint32_t cap, uint32_t *rx_len, uint32_t timeout_ms);
#endif