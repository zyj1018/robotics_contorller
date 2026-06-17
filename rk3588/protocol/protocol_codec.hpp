#pragma once
#include <cstdint>
#include <cstddef>
#include "protocol_frame.h"
namespace robot_controller { namespace protocol {
class Codec {
public:
    static int Serialize(const ProtocolHeader& h, const uint8_t* payload,
                         uint16_t len, uint8_t* buf, size_t sz, size_t* out);
    static int Deserialize(const uint8_t* buf, size_t sz, ProtocolHeader& h,
                           const uint8_t*& payload, uint16_t& len);
};
}}
