#include "Sonic2RingLayout.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>



namespace {

uint16_t readWord(const std::vector<uint8_t>& data, size_t offset)
{
    return static_cast<uint16_t>(data[offset] << 8) | data[offset + 1];
}

}  // namespace

std::vector<RingGroup> Sonic2RingLayout::read(const std::vector<uint8_t>& data)
{
    std::vector<RingGroup> groups;

    for (size_t offset = 0; offset + 1 < data.size();) {
        const uint16_t x = readWord(data, offset);
        offset += 2;

        if (x == 0xffff) {
            return groups;
        }

        if (offset + 1 >= data.size()) {
            throw std::runtime_error("Truncated Sonic 2 ring layout entry");
        }

        const uint16_t encodedY = readWord(data, offset);
        offset += 2;

        groups.push_back({
            x,
            static_cast<uint16_t>(encodedY & 0x0fff),
            static_cast<uint8_t>(((encodedY >> 12) & 0x07) + 1),
            (encodedY & 0x8000) != 0 ? RingDirection::kVertical : RingDirection::kHorizontal
        });
    }

    throw std::runtime_error("Sonic 2 ring layout has no terminator");
}
