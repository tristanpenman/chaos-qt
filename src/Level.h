#pragma once

#include <cstddef>
#include <vector>

#include "RingGroup.h"

class Chunk;
class Block;
class Map;
class Palette;
class Pattern;

class Level
{
public:
    virtual ~Level()
    {
    }

    virtual size_t getPaletteCount() const = 0;
    virtual const Palette& getPalette(size_t index) const = 0;
    virtual Palette& getPalette(size_t index) = 0;

    // 8x8 patterns
    virtual size_t getPatternCount() const = 0;
    virtual const Pattern& getPattern(size_t index) const = 0;
    virtual Pattern& getPattern(size_t index) = 0;

    // 16x16 blocks
    virtual size_t getBlockCount() const = 0;
    virtual const Block& getBlock(size_t index) const = 0;
    virtual Block& getBlock(size_t index) = 0;

    // 128x128 chunks
    virtual size_t getChunkCount() const = 0;
    virtual const Chunk& getChunk(size_t index) const = 0;
    virtual Chunk& getChunk(size_t index) = 0;

    virtual Map& getMap() = 0;

    virtual const std::vector<RingGroup>& getRingGroups() const
    {
        static const std::vector<RingGroup> kEmpty;
        return kEmpty;
    }
};
