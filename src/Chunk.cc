#include "Chunk.h"

#include <cstring>
#include <stdexcept>


void Chunk::fromSegaFormat(uint8_t buffer[kChunkSizeInRom])
{
    for (unsigned int i = 0; i < kBlocksPerChunk; i++) {
        uint16_t index = (static_cast<uint16_t>(buffer[0]) << 8) & 0xFF00;
        index |= (buffer[1]) & 0x00FF;

        blockDescs_[i].set(index);

        buffer += BlockDesc::getIndexSize();
    }
}

void Chunk::toSegaFormat(uint8_t buffer[kChunkSizeInRom]) const
{
    for (unsigned int i = 0; i < kBlocksPerChunk; i++) {
        const uint16_t index = blockDescs_[i].get();
        buffer[0] = static_cast<uint8_t>((index >> 8) & 0xFF);
        buffer[1] = static_cast<uint8_t>(index & 0xFF);
        buffer += BlockDesc::getIndexSize();
    }
}

const BlockDesc& Chunk::getBlockDesc(uint8_t x, uint8_t y) const
{
    if (x > 7 || y > 7) {
        throw std::runtime_error("Invalid block index");
    }

    return blockDescs_[y * 8 + x];
}

void Chunk::setBlockDesc(uint8_t x, uint8_t y, uint16_t value)
{
    if (x > 7 || y > 7) {
        throw std::runtime_error("Invalid block index");
    }

    blockDescs_[y * 8 + x].set(value);
}
