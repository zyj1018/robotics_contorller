#include "mock_canfd.h"
#include <string.h>
void mock_canfd_bus_init(mock_canfd_bus_t *bus) { memset(bus,0,sizeof(*bus)); }
int mock_canfd_bus_write(mock_canfd_bus_t *bus, const mock_canfd_frame_t *f) {
    if(!bus||bus->count>=MOCK_CANFD_RING_SIZE) return -1;
    memcpy(&bus->frames[bus->head],f,sizeof(*f)); bus->head=(bus->head+1)%MOCK_CANFD_RING_SIZE; bus->count++; return 0;
}
int mock_canfd_bus_read(mock_canfd_bus_t *bus, mock_canfd_frame_t *f) {
    if(!bus||bus->count==0) return -1;
    memcpy(f,&bus->frames[bus->tail],sizeof(*f)); bus->tail=(bus->tail+1)%MOCK_CANFD_RING_SIZE; bus->count--; return 0;
}
void mock_canfd_bus_set_bus_off(mock_canfd_bus_t *bus, bool off) { if(bus)bus->bus_off=off; }
void mock_canfd_bus_inject_error(mock_canfd_bus_t *bus, uint32_t flags) { if(bus)bus->error_flags|=flags; }
