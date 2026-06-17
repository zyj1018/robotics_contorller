#include "mock_rs485.h"
#include <string.h>
#include <stdlib.h>
void mock_rs485_bus_init(mock_rs485_bus_t *bus) { memset(bus,0,sizeof(*bus)); }
void mock_rs485_set_response(mock_rs485_bus_t *bus, uint32_t cmd_id, const uint8_t *resp, uint32_t len) {
    for(int i=0;i<MOCK_RS485_RESP_TABLE_SIZE;i++){if(!bus->entries[i].used){bus->entries[i].cmd_id=cmd_id;bus->entries[i].response=malloc(len);memcpy(bus->entries[i].response,resp,len);bus->entries[i].resp_len=len;bus->entries[i].used=true;break;}}
}
void mock_rs485_set_offline(mock_rs485_bus_t *bus, bool offline) { bus->device_offline=offline; }
