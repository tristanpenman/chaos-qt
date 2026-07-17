#include "Sonic3Level.h"

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


#undef LOG
#define LOG Logger("Sonic3Level")


namespace {

constexpr uint8_t kMapLayers = 2;

}  // namespace

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
    if (index >= kPaletteCount) {
        throw std::runtime_error("Invalid palette index");
    }

    return palettes_[index];
}

Palette& Sonic3Level::getPalette(size_t index)
{
    if (index >= kPaletteCount) {
        throw std::runtime_error("Invalid palette index");
    }

    return palettes_[index];
}

const Pattern& Sonic3Level::getPattern(size_t index) const
{
    if (index >= patternCount_) {
        throw std::runtime_error("Invalid pattern index");
    }

    return patterns_[index];
}

Pattern& Sonic3Level::getPattern(size_t index)
{
    if (index >= patternCount_) {
        throw std::runtime_error("Invalid pattern index");
    }

    return patterns_[index];
}

const Block& Sonic3Level::getBlock(size_t index) const
{
    if (index >= blockCount_) {
        throw std::runtime_error("Invalid block index");
    }

    return blocks_[index];
}

Block& Sonic3Level::getBlock(size_t index)
{
    if (index >= blockCount_) {
        throw std::runtime_error("Invalid block index");
    }

    return blocks_[index];
}

const Chunk& Sonic3Level::getChunk(size_t index) const
{
    if (index >= chunkCount_) {
        throw std::runtime_error("Invalid chunk index");
    }

    return chunks_[index];
}

Chunk& Sonic3Level::getChunk(size_t index)
{
    if (index >= chunkCount_) {
        throw std::runtime_error("Invalid chunk index");
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
        auto buffer = rom.readBytes(characterPaletteAddr, Palette::kPaletteSizeInRom);
        palettes_[0].fromSegaFormat(buffer.data());
    }

    {
        auto buffer = rom.readBytes(levelPalettesAddr, Palette::kPaletteSizeInRom * 3);
        for (int i = 0; i < 3; i++) {
            palettes_[i + 1].fromSegaFormat(&buffer[Palette::kPaletteSizeInRom * i]);
        }
    }
}

void Sonic3Level::loadPatterns(Rom& rom, uint32_t basePatternsAddr, uint32_t extPatternsAddr)
{
    static constexpr size_t kPatternBufferSize = 0xFFFFF;  // 64KB

    // Length of uncompressed data
    const uint16_t baseDataSize = rom.read16BitAddr(basePatternsAddr);
    const uint16_t extDataSize = rom.read16BitAddr(extPatternsAddr);

    // Total number of patterns
    patternCount_ = (baseDataSize + extDataSize) / Pattern::kPatternSizeInRom;
    patterns_ = new Pattern[patternCount_];

    auto& file = rom.getFile();
    KosinskiReader reader;
    std::vector<uint8_t> buffer(kPatternBufferSize);
    size_t total = 0;
    int patternIndex = 0;

    {
        // Base patterns
        file.seek(basePatternsAddr + 2);
        while (total < baseDataSize) {
            // Decompress module
            auto result = reader.decompress(file, buffer.data(), kPatternBufferSize);
            if (!result.first) {
                throw std::runtime_error("Base pattern decompression error");
            }

            if (result.second % Pattern::kPatternSizeInRom != 0) {
                throw std::runtime_error("Inconsistent base pattern data");
            }

            const auto patternCount = result.second / Pattern::kPatternSizeInRom;
            for (size_t i = 0; i < patternCount; i++) {
                patterns_[patternIndex++].fromSegaFormat(&buffer[i * Pattern::kPatternSizeInRom]);
            }

            // Find the beginning of the next module...
            // Modules are padded with zeroes
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
        // Extended patterns
        file.seek(extPatternsAddr + 2);
        while (total < baseDataSize + extDataSize) {
            auto result = reader.decompress(file, buffer.data(), kPatternBufferSize);
            if (!result.first) {
                throw std::runtime_error("Extended pattern decompression error");
            }

            if (result.second % Pattern::kPatternSizeInRom != 0) {
                throw std::runtime_error("Inconsistent extended pattern data");
            }

            const auto patternCount = result.second / Pattern::kPatternSizeInRom;
            for (size_t i = 0; i < patternCount; i++) {
                patterns_[patternIndex++].fromSegaFormat(&buffer[i * Pattern::kPatternSizeInRom]);
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
    static constexpr size_t kBlockBufferSize = 0xFFFF;  // 64KB

    auto& file = rom.getFile();
    KosinskiReader reader;
    std::vector<uint8_t> buffer(kBlockBufferSize);
    size_t total = 0;

    {
        // Decompress base blocks
        file.seek(baseBlocksAddr);
        auto result = reader.decompress(file, buffer.data(), kBlockBufferSize);
        if (!result.first) {
            throw std::runtime_error("Base block decompression error");
        }

        if (result.second % Block::kBlockSizeInRom != 0) {
            throw std::runtime_error("Inconsistent base block data");
        }

        blockCount_ = result.second / Block::kBlockSizeInRom;
        total += result.second;
    }

    {
        // Decompress extended blocks
        file.seek(extBlocksAddr);
        auto result = reader.decompress(file, buffer.data() + total, kBlockBufferSize - total);
        if (!result.first) {
            throw std::runtime_error("Extended block decompression error");
        }

        if (result.second % Block::kBlockSizeInRom != 0) {
            throw std::runtime_error("Inconsistent extended block data");
        }

        blockCount_ += result.second / Block::kBlockSizeInRom;
        blocks_ = new Block[blockCount_];

        for (size_t i = 0; i < blockCount_; i++) {
            blocks_[i].fromSegaFormat(&buffer[i * Block::kBlockSizeInRom]);
        }

        total += result.second;
    }

    LOG() << "Block count: " << blockCount_ << " (total: " << total << " bytes)";
}

void Sonic3Level::loadChunks(Rom& rom, uint32_t baseChunksAddr, uint32_t extChunksAddr)
{
    static constexpr size_t kChunkBufferSize = 0xFFFFF;  // 64KB

    auto& file = rom.getFile();
    KosinskiReader reader;
    std::vector<uint8_t> buffer(kChunkBufferSize);
    size_t total = 0;

    {
        // Decompress base chunks
        file.seek(baseChunksAddr);
        auto result = reader.decompress(file, buffer.data(), kChunkBufferSize);
        if (!result.first) {
            throw std::runtime_error("Base chunk decompression error");
        }

        if (result.second % Chunk::kChunkSizeInRom != 0) {
            throw std::runtime_error("Inconsistent base chunk data");
        }

        chunkCount_ = result.second / Chunk::kChunkSizeInRom;
        total += result.second;
    }

    {
        // Decompress extended chunks
        file.seek(extChunksAddr);
        auto result = reader.decompress(file, buffer.data() + total, kChunkBufferSize - total);
        if (!result.first) {
            throw std::runtime_error("Extended chunk decompression error");
        }

        if (result.second % Chunk::kChunkSizeInRom != 0) {
            throw std::runtime_error("Inconsistent extended chunk data");
        }

        chunkCount_ += result.second / Chunk::kChunkSizeInRom;
        chunks_ = new Chunk[chunkCount_];

        for (size_t i = 0; i < chunkCount_; i++) {
            chunks_[i].fromSegaFormat(&buffer[i * Chunk::kChunkSizeInRom]);
        }
    }

    LOG() << "Chunk count: " << chunkCount_ << " (total: " << total << " bytes)";
}

void Sonic3Level::loadMap(Rom& rom, uint32_t mapAddr)
{
    // Read map header
    const uint16_t rowSizeFg = rom.read16BitAddr(mapAddr);
    const uint16_t rowSizeBg = rom.read16BitAddr(mapAddr + 2);
    const uint16_t rowCountFg = rom.read16BitAddr(mapAddr + 4);
    const uint16_t rowCountBg = rom.read16BitAddr(mapAddr + 6);

    // Create map
    const uint16_t mapWidth = std::max(rowSizeBg, rowSizeFg);
    const uint16_t mapHeight = std::max(rowCountBg, rowCountFg);
    map_ = new Map(kMapLayers, mapWidth, mapHeight);

    // Setup for reading values
    auto& file = rom.getFile();
    const size_t bufferSize = sizeof(uint8_t) * mapWidth;
    std::vector<uint8_t> buffer(bufferSize);
    const uint32_t ptrTableAddr = mapAddr + 8;

    // Read rows
    for (uint16_t rowIndex = 0; rowIndex < rowCountFg; rowIndex++) {
        const qint64 rowOffset = rom.read16BitAddr(ptrTableAddr + rowIndex * 4) - 0x8000;
        file.seek(ptrTableAddr + rowOffset);
        file.read(reinterpret_cast<char*>(buffer.data()), bufferSize);

        // Set tiles
        for (uint16_t colIndex = 0; colIndex < rowSizeFg; colIndex++) {
            map_->setValue(0, colIndex, rowIndex, buffer[colIndex]);
        }
    }

    LOG() << "Map size: " << mapWidth << "x" << mapHeight;
}
