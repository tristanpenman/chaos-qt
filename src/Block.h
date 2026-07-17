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
    static constexpr uint8_t kBlockHeight = 16;
    static constexpr uint8_t kBlockWidth = 16;
    static constexpr uint8_t kPatternsPerBlock = 4;
    static constexpr uint8_t kBytesPerPattern = 2;
    static constexpr uint8_t kBlockSizeInRom = kPatternsPerBlock * kBytesPerPattern;

    Block() = default;

    void fromSegaFormat(uint8_t buffer[kBlockSizeInRom]);
    void toSegaFormat(uint8_t buffer[kBlockSizeInRom]) const;

    const PatternDesc& getPatternDesc(uint8_t x, uint8_t y) const;
    void setPatternDesc(uint8_t x, uint8_t y, uint16_t value);

private:
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    std::array<PatternDesc, kPatternsPerBlock> patternDescs_;
};
