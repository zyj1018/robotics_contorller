#ifndef MOCK_RS485_H
#define MOCK_RS485_H
#include <stdint.h>
#define MOCK_RS485_RESP_TABLE_SIZE 16
typedef struct {
    uint32_t cmd_id; uint8_t *response; uint32_t resp_len; bool used;
} mock_rs485_resp_entry_t;
typedef struct {
    mock_rs485_resp_entry_t entries[MOCK_RS485_RESP_TABLE_SIZE];
    uint8_t *last_tx; uint32_t last_tx_len;
    uint8_t *next_rx; uint32_t next_rx_len;
    bool device_offline; bool bus_busy; uint32_t error_flags;
} mock_rs485_bus_t;
void mock_rs485_bus_init(mock_rs485_bus_t *bus);
void mock_rs485_set_response(mock_rs485_bus_t *bus, uint32_t cmd_id, const uint8_t *resp, uint32_t len);
void mock_rs485_set_offline(mock_rs485_bus_t *bus, bool offline);
#endif
