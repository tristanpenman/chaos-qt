#pragma once

#include <cstdint>

enum class RingDirection
{
    kHorizontal,
    kVertical
};

struct RingGroup
{
    uint16_t x;
    uint16_t y;
    uint8_t count;
    RingDirection direction;
};
