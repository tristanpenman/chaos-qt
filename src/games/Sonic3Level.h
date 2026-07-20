#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "../Level.h"

class Rom;

class Sonic3Level : public Level
{
    static constexpr size_t kPaletteCount = 4;

public:
    Sonic3Level(Rom& rom,
                uint32_t characterPaletteAddr,
                uint32_t levelPalettesAddr,
                uint32_t patternsAddr,
                uint32_t extendedPatternsAddr,
                uint32_t blocksAddr,
                uint32_t extendedBlocksAddr,
                uint32_t chunksAddr,
                uint32_t extendedChunksAddr,
                uint32_t mapAddr);
    ~Sonic3Level() override;

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

private:
    Sonic3Level(const Sonic3Level&) = delete;
    Sonic3Level& operator=(const Sonic3Level&) = delete;

    void loadPalettes(Rom& rom, uint32_t characterPaletteAddr, uint32_t levelPalettesAddr);
    void loadPatterns(Rom& rom, uint32_t patternsAddr, uint32_t extendedPatternsAddr);
    void loadBlocks(Rom& rom, uint32_t blocksAddr, uint32_t extendedBlocksAddr);
    void loadChunks(Rom& rom, uint32_t chunksAddr, uint32_t extendedChunksAddr);
    void loadMap(Rom& rom, uint32_t mapAddr);

    std::unique_ptr<Palette[]> palettes_;
    std::unique_ptr<Pattern[]> patterns_;
    std::unique_ptr<Block[]> blocks_;
    std::unique_ptr<Chunk[]> chunks_;
    std::unique_ptr<Map> map_;

    size_t patternCount_;
    size_t blockCount_;
    size_t chunkCount_;
};

inline size_t Sonic3Level::getPaletteCount() const
{
    return kPaletteCount;
}

inline size_t Sonic3Level::getPatternCount() const
{
    return patternCount_;
}

inline size_t Sonic3Level::getBlockCount() const
{
    return blockCount_;
}

inline size_t Sonic3Level::getChunkCount() const
{
    return chunkCount_;
}
