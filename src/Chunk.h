#pragma once

#include <array>
#include <cstdint>

#include "BlockDesc.h"

class Block;

class Chunk
{
public:
    static constexpr uint8_t CHUNK_HEIGHT = 128;
    static constexpr uint8_t CHUNK_WIDTH = 128;
    static constexpr uint8_t BYTES_PER_BLOCK = 2;
    static constexpr uint8_t BLOCKS_PER_CHUNK = 64;
    static constexpr uint8_t CHUNK_SIZE_IN_ROM = BLOCKS_PER_CHUNK * BYTES_PER_BLOCK;

    Chunk() = default;

    void fromSegaFormat(uint8_t buffer[CHUNK_SIZE_IN_ROM]);
    void toSegaFormat(uint8_t buffer[CHUNK_SIZE_IN_ROM]) const;

    const BlockDesc& getBlockDesc(uint8_t x, uint8_t y) const;
    void setBlockDesc(uint8_t x, uint8_t y, uint16_t value);

private:
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    std::array<BlockDesc, BLOCKS_PER_CHUNK> m_blockDescs;
};
