#pragma once

#include <array>
#include <cstdint>

#include "PatternDesc.h"

/**
 * Representation of a 16x16 tile, composed of 4 8x8 SEGA patterns
 *
 * Pattern are defined using a common descriptor format, which includes properties such as how
 * the pattern is flipped. See PatternDesc.h for more information.
 */
class Block
{
public:
    static constexpr uint8_t BLOCK_HEIGHT = 16;
    static constexpr uint8_t BLOCK_WIDTH = 16;
    static constexpr uint8_t PATTERNS_PER_BLOCK = 4;
    static constexpr uint8_t BYTES_PER_PATTERN = 2;
    static constexpr uint8_t BLOCK_SIZE_IN_ROM = PATTERNS_PER_BLOCK * BYTES_PER_PATTERN;

    Block() = default;

    void fromSegaFormat(uint8_t buffer[BLOCK_SIZE_IN_ROM]);
    void toSegaFormat(uint8_t buffer[BLOCK_SIZE_IN_ROM]) const;

    const PatternDesc& getPatternDesc(uint8_t x, uint8_t y) const;
    void setPatternDesc(uint8_t x, uint8_t y, uint16_t value);

private:
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    std::array<PatternDesc, PATTERNS_PER_BLOCK> patternDescs_;
};
