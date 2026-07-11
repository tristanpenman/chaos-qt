#include <cstring>
#include <stdexcept>

#include "Chunk.h"

void Chunk::fromSegaFormat(uint8_t buffer[CHUNK_SIZE_IN_ROM])
{
    for (unsigned int i = 0; i < BLOCKS_PER_CHUNK; i++) {
        uint16_t index = (static_cast<uint16_t>(buffer[0]) << 8) & 0xFF00;
        index |= (buffer[1]) & 0x00FF;

        m_blockDescs[i].set(index);

        buffer += BlockDesc::getIndexSize();
    }
}

void Chunk::toSegaFormat(uint8_t buffer[CHUNK_SIZE_IN_ROM]) const
{
    for (unsigned int i = 0; i < BLOCKS_PER_CHUNK; i++) {
        const uint16_t index = m_blockDescs[i].get();
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

    return m_blockDescs[y * 8 + x];
}

void Chunk::setBlockDesc(uint8_t x, uint8_t y, uint16_t value)
{
    if (x > 7 || y > 7) {
        throw std::runtime_error("Invalid block index");
    }

    m_blockDescs[y * 8 + x].set(value);
}
