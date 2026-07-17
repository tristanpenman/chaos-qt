#include <cstring>
#include <stdexcept>

#include "Pattern.h"

#include "Block.h"

void Block::fromSegaFormat(uint8_t buffer[BLOCK_SIZE_IN_ROM])
{
    for (unsigned int i = 0; i < PATTERNS_PER_BLOCK; i++) {
        uint16_t index = (static_cast<uint16_t>(buffer[0]) << 8) & 0xFF00;
        index |= (buffer[1]) & 0x00FF;

        // Set index
        patternDescs_[i].set(index);

        buffer += PatternDesc::getIndexSize();
    }
}

void Block::toSegaFormat(uint8_t buffer[BLOCK_SIZE_IN_ROM]) const
{
    for (unsigned int i = 0; i < PATTERNS_PER_BLOCK; i++) {
        const uint16_t index = patternDescs_[i].get();
        buffer[0] = static_cast<uint8_t>((index >> 8) & 0xFF);
        buffer[1] = static_cast<uint8_t>(index & 0xFF);
        buffer += PatternDesc::getIndexSize();
    }
}

const PatternDesc& Block::getPatternDesc(uint8_t x, uint8_t y) const
{
    if (x > 1 || y > 1) {
        throw std::runtime_error("Invalid pattern index");
    }

    return patternDescs_[y * 2 + x];
}

void Block::setPatternDesc(uint8_t x, uint8_t y, uint16_t value)
{
    if (x > 1 || y > 1) {
        throw std::runtime_error("Invalid pattern index");
    }

    patternDescs_[y * 2 + x].set(value);
}
