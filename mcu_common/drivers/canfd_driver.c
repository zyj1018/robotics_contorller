#include "canfd_driver.h"
#include <string.h>
#ifdef DRIVER_BACKEND_MOCK
#include "mock_canfd.h"
#endif
int canfd_channel_init(canfd_channel_t *ch, uint8_t id) { ch->channel_id=id; ch->state=CANFD_STATE_ACTIVE; memset(&ch->stats,0,sizeof(ch->stats)); return 0; }
int canfd_channel_send(canfd_channel_t *ch, const canfd_frame_t *frame) { (void)ch;(void)frame; return 0; }
int canfd_channel_receive(canfd_channel_t *ch, canfd_frame_t *frame, uint32_t timeout_ms) { (void)ch;(void)frame;(void)timeout_ms; return -1; }
int canfd_channel_recover_bus_off(canfd_channel_t *ch) { ch->state=CANFD_STATE_ACTIVE; return 0; }
int canfd_channel_get_stats(canfd_channel_t *ch, canfd_stats_t *stats) { memcpy(stats,&ch->stats,sizeof(*stats)); return 0; }
