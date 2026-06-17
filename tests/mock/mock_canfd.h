#ifndef MOCK_CANFD_H
#define MOCK_CANFD_H
#include <stdint.h>
#include <stdbool.h>
#define MOCK_CANFD_RING_SIZE 64
typedef struct { uint32_t id; bool extended; bool fd_format; uint8_t dlc; uint8_t data[64]; } mock_canfd_frame_t;
typedef struct {
    mock_canfd_frame_t frames[MOCK_CANFD_RING_SIZE];
    uint8_t head, tail, count; bool bus_off; uint32_t error_flags;
} mock_canfd_bus_t;
void mock_canfd_bus_init(mock_canfd_bus_t *bus);
int mock_canfd_bus_write(mock_canfd_bus_t *bus, const mock_canfd_frame_t *frame);
int mock_canfd_bus_read(mock_canfd_bus_t *bus, mock_canfd_frame_t *frame);
void mock_canfd_bus_set_bus_off(mock_canfd_bus_t *bus, bool off);
void mock_canfd_bus_inject_error(mock_canfd_bus_t *bus, uint32_t flags);
#endif
