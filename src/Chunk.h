#pragma once

#include <array>
#include <cstdint>

#include "BlockDesc.h"

class Block;

class Chunk
{
public:
    static constexpr uint8_t kChunkHeight = 128;
    static constexpr uint8_t kChunkWidth = 128;
    static constexpr uint8_t kBytesPerBlock = 2;
    static constexpr uint8_t kBlocksPerChunk = 64;
    static constexpr uint8_t kChunkSizeInRom = kBlocksPerChunk * kBytesPerBlock;

    Chunk() = default;

    void fromSegaFormat(uint8_t buffer[kChunkSizeInRom]);
    void toSegaFormat(uint8_t buffer[kChunkSizeInRom]) const;

    const BlockDesc& getBlockDesc(uint8_t x, uint8_t y) const;
    void setBlockDesc(uint8_t x, uint8_t y, uint16_t value);

private:
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    std::array<BlockDesc, kBlocksPerChunk> blockDescs_;
};
