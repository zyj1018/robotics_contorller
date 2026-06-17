#include "protocol_codec.hpp"
#include "protocol_codec.h"
#include <cstring>

namespace robot_controller {
namespace protocol {

int Codec::Serialize(const ProtocolHeader& h, const uint8_t* payload,
                     uint16_t len, uint8_t* buf, size_t sz, size_t* out) {
    return protocol_frame_serialize(&h, payload, len, buf, sz, out);
}

int Codec::Deserialize(const uint8_t* buf, size_t sz, ProtocolHeader& h,
                       const uint8_t*& payload, uint16_t& len) {
    return protocol_frame_deserialize(buf, sz, &h, &payload, &len);
}

} // namespace protocol
} // namespace robot_controller
