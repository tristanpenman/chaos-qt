#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "../Block.h"
#include "../Chunk.h"
#include "../KosinskiReader.h"
#include "../Logger.h"
#include "../Map.h"
#include "../Palette.h"
#include "../Pattern.h"
#include "../Rom.h"

#include "Sonic3Level.h"

#undef LOG
#define LOG Logger("Sonic3Level")

using namespace std;

static constexpr uint8_t MAP_LAYERS = 2;

Sonic3Level::Sonic3Level(Rom& rom,
                         uint32_t sonicPaletteAddr,
                         uint32_t levelPalettesAddr,
                         uint32_t patternsAddr,
                         uint32_t extendedPatternsAddr,
                         uint32_t blocksAddr,
                         uint32_t extendedBlocksAddr,
                         uint32_t chunksAddr,
                         uint32_t extendedChunksAddr,
                         uint32_t mapAddr)
    : palettes_(nullptr)
    , patterns_(nullptr)
    , blocks_(nullptr)
    , map_(nullptr)
    , patternCount_(0)
    , blockCount_(0)
    , chunkCount_(0)
{
    loadPalettes(rom, sonicPaletteAddr, levelPalettesAddr);
    loadPatterns(rom, patternsAddr, extendedPatternsAddr);
    loadBlocks(rom, blocksAddr, extendedBlocksAddr);
    loadChunks(rom, chunksAddr, extendedChunksAddr);
    loadMap(rom, mapAddr);
}

const Palette& Sonic3Level::getPalette(size_t index) const
{
    if (index >= PALETTE_COUNT) {
        throw runtime_error("Invalid palette index");
    }

    return palettes_[index];
}

Palette& Sonic3Level::getPalette(size_t index)
{
    if (index >= PALETTE_COUNT) {
        throw runtime_error("Invalid palette index");
    }

    return palettes_[index];
}

const Pattern& Sonic3Level::getPattern(size_t index) const
{
    if (index >= patternCount_) {
        throw runtime_error("Invalid pattern index");
    }

    return patterns_[index];
}

Pattern& Sonic3Level::getPattern(size_t index)
{
    if (index >= patternCount_) {
        throw runtime_error("Invalid pattern index");
    }

    return patterns_[index];
}

const Block& Sonic3Level::getBlock(size_t index) const
{
    if (index >= blockCount_) {
        throw runtime_error("Invalid block index");
    }

    return blocks_[index];
}

Block& Sonic3Level::getBlock(size_t index)
{
    if (index >= blockCount_) {
        throw runtime_error("Invalid block index");
    }

    return blocks_[index];
}

const Chunk& Sonic3Level::getChunk(size_t index) const
{
    if (index >= chunkCount_) {
        throw runtime_error("Invalid chunk index");
    }

    return chunks_[index];
}

Chunk& Sonic3Level::getChunk(size_t index)
{
    if (index >= chunkCount_) {
        throw runtime_error("Invalid chunk index");
    }

    return chunks_[index];
}

Map& Sonic3Level::getMap()
{
    return *map_;
}

void Sonic3Level::loadPalettes(Rom& rom, uint32_t characterPaletteAddr, uint32_t levelPalettesAddr)
{
    palettes_ = new Palette[4];

    {
        auto buffer = rom.readBytes(characterPaletteAddr, Palette::PALETTE_SIZE_IN_ROM);
        palettes_[0].fromSegaFormat(buffer.data());
    }

    {
        auto buffer = rom.readBytes(levelPalettesAddr, Palette::PALETTE_SIZE_IN_ROM * 3);
        for (int i = 0; i < 3; i++) {
            palettes_[i + 1].fromSegaFormat(&buffer[Palette::PALETTE_SIZE_IN_ROM * i]);
        }
    }
}

void Sonic3Level::loadPatterns(Rom& rom, uint32_t basePatternsAddr, uint32_t extPatternsAddr)
{
    static constexpr size_t PATTERN_BUFFER_SIZE = 0xFFFFF;  // 64KB

    // length of uncompressed data
    const uint16_t baseDataSize = rom.read16BitAddr(basePatternsAddr);
    const uint16_t extDataSize = rom.read16BitAddr(extPatternsAddr);

    // total number of patterns
    patternCount_ = (baseDataSize + extDataSize) / Pattern::PATTERN_SIZE_IN_ROM;
    patterns_ = new Pattern[patternCount_];

    // setup decompression
    auto& file = rom.getFile();
    KosinskiReader reader;
    vector<uint8_t> buffer(PATTERN_BUFFER_SIZE);
    size_t total = 0;
    int patternIndex = 0;

    {
        // base patterns
        file.seek(basePatternsAddr + 2);
        while (total < baseDataSize) {
            // decompress module
            auto result = reader.decompress(file, buffer.data(), PATTERN_BUFFER_SIZE);
            if (!result.first) {
                throw runtime_error("Base pattern decompression error");
            }

            if (result.second % Pattern::PATTERN_SIZE_IN_ROM != 0) {
                throw runtime_error("Inconsistent base pattern data");
            }

            const auto patternCount = result.second / Pattern::PATTERN_SIZE_IN_ROM;
            for (size_t i = 0; i < patternCount; i++) {
                patterns_[patternIndex++].fromSegaFormat(&buffer[i * Pattern::PATTERN_SIZE_IN_ROM]);
            }

            // Find the beginning of the next module...
            // modules are padded with zeroes
            char b = 0;
            while (b == 0) {
                file.getChar(&b);
            }

            // Set read address to the next packet/module
            file.seek(file.pos() - 1);

            total += result.second;
        }
    }

    {
        // extended patterns
        file.seek(extPatternsAddr + 2);
        while (total < baseDataSize + extDataSize) {
            auto result = reader.decompress(file, buffer.data(), PATTERN_BUFFER_SIZE);
            if (!result.first) {
                throw runtime_error("Extended pattern decompression error");
            }

            if (result.second % Pattern::PATTERN_SIZE_IN_ROM != 0) {
                throw runtime_error("Inconsistent extended pattern data");
            }

            const auto patternCount = result.second / Pattern::PATTERN_SIZE_IN_ROM;
            for (size_t i = 0; i < patternCount; i++) {
                patterns_[patternIndex++].fromSegaFormat(&buffer[i * Pattern::PATTERN_SIZE_IN_ROM]);
            }

            char b = 0;
            while (b == 0) {
                file.getChar(&b);
            }

            // Set read address to the next packet/module
            file.seek(file.pos() - 1);

            total += result.second;
        }
    }

    LOG() << "Pattern count: " << patternCount_ << " (total: " << total << " bytes)";
}

void Sonic3Level::loadBlocks(Rom& rom, uint32_t baseBlocksAddr, uint32_t extBlocksAddr)
{
    static constexpr size_t BLOCK_BUFFER_SIZE = 0xFFFF;  // 64KB

    // setup decompression
    auto& file = rom.getFile();
    KosinskiReader reader;
    vector<uint8_t> buffer(BLOCK_BUFFER_SIZE);
    size_t total = 0;

    {
        // decompress base blocks
        file.seek(baseBlocksAddr);
        auto result = reader.decompress(file, buffer.data(), BLOCK_BUFFER_SIZE);
        if (!result.first) {
            throw runtime_error("Base block decompression error");
        }

        if (result.second % Block::BLOCK_SIZE_IN_ROM != 0) {
            throw runtime_error("Inconsistent base block data");
        }

        blockCount_ = result.second / Block::BLOCK_SIZE_IN_ROM;
        total += result.second;
    }

    {
        // decompress extended blocks
        file.seek(extBlocksAddr);
        auto result = reader.decompress(file, buffer.data() + total, BLOCK_BUFFER_SIZE - total);
        if (!result.first) {
            throw runtime_error("Extended block decompression error");
        }

        if (result.second % Block::BLOCK_SIZE_IN_ROM != 0) {
            throw runtime_error("Inconsistent extended block data");
        }

        blockCount_ += result.second / Block::BLOCK_SIZE_IN_ROM;
        blocks_ = new Block[blockCount_];

        for (size_t i = 0; i < blockCount_; i++) {
            blocks_[i].fromSegaFormat(&buffer[i * Block::BLOCK_SIZE_IN_ROM]);
        }

        total += result.second;
    }

    LOG() << "Block count: " << blockCount_ << " (total: " << total << " bytes)";
}

void Sonic3Level::loadChunks(Rom& rom, uint32_t baseChunksAddr, uint32_t extChunksAddr)
{
    static constexpr size_t CHUNK_BUFFER_SIZE = 0xFFFFF;  // 64KB

    // setup decompression
    auto& file = rom.getFile();
    KosinskiReader reader;
    vector<uint8_t> buffer(CHUNK_BUFFER_SIZE);
    size_t total = 0;

    {
        // decompress base chunks
        file.seek(baseChunksAddr);
        auto result = reader.decompress(file, buffer.data(), CHUNK_BUFFER_SIZE);
        if (!result.first) {
            throw runtime_error("Base chunk decompression error");
        }

        if (result.second % Chunk::CHUNK_SIZE_IN_ROM != 0) {
            throw runtime_error("Inconsistent base chunk data");
        }

        chunkCount_ = result.second / Chunk::CHUNK_SIZE_IN_ROM;
        total += result.second;
    }

    {
        // decompress extended chunks
        file.seek(extChunksAddr);
        auto result = reader.decompress(file, buffer.data() + total, CHUNK_BUFFER_SIZE - total);
        if (!result.first) {
            throw runtime_error("Extended chunk decompression error");
        }

        if (result.second % Chunk::CHUNK_SIZE_IN_ROM != 0) {
            throw runtime_error("Inconsistent extended chunk data");
        }

        chunkCount_ += result.second / Chunk::CHUNK_SIZE_IN_ROM;
        chunks_ = new Chunk[chunkCount_];

        for (size_t i = 0; i < chunkCount_; i++) {
            chunks_[i].fromSegaFormat(&buffer[i * Chunk::CHUNK_SIZE_IN_ROM]);
        }
    }

    LOG() << "Chunk count: " << chunkCount_ << " (total: " << total << " bytes)";
}

void Sonic3Level::loadMap(Rom& rom, uint32_t mapAddr)
{
    // read map header
    const uint16_t rowSizeFg = rom.read16BitAddr(mapAddr);
    const uint16_t rowSizeBg = rom.read16BitAddr(mapAddr + 2);
    const uint16_t rowCountFg = rom.read16BitAddr(mapAddr + 4);
    const uint16_t rowCountBg = rom.read16BitAddr(mapAddr + 6);

    // create map
    const uint16_t mapWidth = max(rowSizeBg, rowSizeFg);
    const uint16_t mapHeight = max(rowCountBg, rowCountFg);
    map_ = new Map(MAP_LAYERS, mapWidth, mapHeight);

    // setup for reading values
    auto& file = rom.getFile();
    const size_t bufferSize = sizeof(uint8_t) * mapWidth;
    vector<uint8_t> buffer(bufferSize);
    const uint32_t ptrTableAddr = mapAddr + 8;

    // read rows
    for (uint16_t rowIndex = 0; rowIndex < rowCountFg; rowIndex++) {
        const qint64 rowOffset = rom.read16BitAddr(ptrTableAddr + rowIndex * 4) - 0x8000;
        file.seek(ptrTableAddr + rowOffset);
        file.read(reinterpret_cast<char*>(buffer.data()), bufferSize);

        // set tiles
        for (uint16_t colIndex = 0; colIndex < rowSizeFg; colIndex++) {
            map_->setValue(0, colIndex, rowIndex, buffer[colIndex]);
        }
    }

    LOG() << "Map size: " << mapWidth << "x" << mapHeight;
}
