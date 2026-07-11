#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../Level.h"

class Chunk;
class Block;
class Map;
class Palette;
class Pattern;
class Rom;

class Sonic2Level : public Level
{
    static constexpr size_t PALETTE_COUNT = 4;

public:
    Sonic2Level(Rom& rom,
                uint32_t characterPaletteAddr,
                uint32_t levelPalettesAddr,
                uint32_t patternsAddr,
                uint32_t blocksAddr,
                uint32_t chunksAddr,
                uint32_t mapAddr,
                uint32_t ringsAddr,
                size_t ringsSize);
    Sonic2Level(const std::vector<char>& paletteData,
                const std::vector<uint8_t>& patternData,
                const std::vector<uint8_t>& blockData,
                const std::vector<uint8_t>& chunkData,
                const std::vector<uint8_t>& mapData,
                const std::vector<uint8_t>& ringData = {});
    ~Sonic2Level() override;

    size_t getPaletteCount() const override;
    const Palette& getPalette(size_t index) const override;
    Palette& getPalette(size_t index) override;

    size_t getPatternCount() const override;
    const Pattern& getPattern(size_t index) const override;
    Pattern& getPattern(size_t index) override;

    size_t getBlockCount() const override;
    const Block& getBlock(size_t index) const override;
    Block& getBlock(size_t index) override;

    size_t getChunkCount() const override;
    const Chunk& getChunk(size_t index) const override;
    Chunk& getChunk(size_t index) override;

    Map& getMap() override;
    const std::vector<RingGroup>& getRingGroups() const override;

private:
    Sonic2Level(const Sonic2Level&) = delete;
    Sonic2Level& operator=(const Sonic2Level&) = delete;

    void loadPalettes(Rom& rom, uint32_t characterPaletteAddr, uint32_t levelPalettesAddr);
    void loadPalettes(const std::vector<char>& data);
    void loadPatterns(Rom& rom, uint32_t patternsAddr);
    void loadPatterns(const std::vector<uint8_t>& data);
    void loadBlocks(Rom& rom, uint32_t blocksAddr);
    void loadBlocks(const std::vector<uint8_t>& data);
    void loadChunks(Rom& rom, uint32_t chunksAddr);
    void loadChunks(const std::vector<uint8_t>& data);
    void loadMap(Rom& rom, uint32_t mapAddr);
    void loadMap(const std::vector<uint8_t>& data);
    void loadRings(Rom& rom, uint32_t ringsAddr, size_t ringsSize);
    void loadRings(const std::vector<uint8_t>& data);

    Palette* m_palettes;
    Pattern* m_patterns;
    Block* m_blocks;
    Chunk* m_chunks;
    Map* m_map;
    std::vector<RingGroup> m_ringGroups;

    size_t m_patternCount;
    size_t m_blockCount;
    size_t m_chunkCount;
};

inline size_t Sonic2Level::getPaletteCount() const
{
    return PALETTE_COUNT;
}

inline size_t Sonic2Level::getPatternCount() const
{
    return m_patternCount;
}

inline size_t Sonic2Level::getBlockCount() const
{
    return m_blockCount;
}

inline size_t Sonic2Level::getChunkCount() const
{
    return m_chunkCount;
}
