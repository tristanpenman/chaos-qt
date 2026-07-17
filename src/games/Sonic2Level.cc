#include "Sonic2Level.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "../Block.h"
#include "../Chunk.h"
#include "../KosinskiReader.h"
#include "../Logger.h"
#include "../Map.h"
#include "../Palette.h"
#include "../Pattern.h"
#include "../Rom.h"

#include "Sonic2RingLayout.h"

#undef LOG
#define LOG Logger("Sonic2Level")

namespace {

constexpr uint8_t kMapLayers = 2;
constexpr uint8_t kMapHeight = 16;
constexpr uint8_t kMapWidth = 128;

}  // namespace

Sonic2Level::Sonic2Level(Rom& rom,
                         uint32_t sonicPaletteAddr,
                         uint32_t levelPalettesAddr,
                         uint32_t patternsAddr,
                         uint32_t blocksAddr,
                         uint32_t chunksAddr,
                         uint32_t mapAddr,
                         uint32_t ringsAddr,
                         size_t ringsSize)
    : palettes_(nullptr)
    , patterns_(nullptr)
    , blocks_(nullptr)
    , chunks_(nullptr)
    , map_(nullptr)
    , patternCount_(0)
    , blockCount_(0)
    , chunkCount_(0)
{
    loadPalettes(rom, sonicPaletteAddr, levelPalettesAddr);
    loadPatterns(rom, patternsAddr);
    loadBlocks(rom, blocksAddr);
    loadChunks(rom, chunksAddr);
    loadMap(rom, mapAddr);
    loadRings(rom, ringsAddr, ringsSize);
}

Sonic2Level::Sonic2Level(const std::vector<char>& paletteData,
                         const std::vector<uint8_t>& patternData,
                         const std::vector<uint8_t>& blockData,
                         const std::vector<uint8_t>& chunkData,
                         const std::vector<uint8_t>& mapData,
                         const std::vector<uint8_t>& ringData)
    : palettes_(nullptr)
    , patterns_(nullptr)
    , blocks_(nullptr)
    , chunks_(nullptr)
    , map_(nullptr)
    , patternCount_(0)
    , blockCount_(0)
    , chunkCount_(0)
{
    loadPalettes(paletteData);
    loadPatterns(patternData);
    loadBlocks(blockData);
    loadChunks(chunkData);
    loadMap(mapData);
    if (!ringData.empty()) {
        loadRings(ringData);
    }
}

Sonic2Level::~Sonic2Level()
{
    delete[] palettes_;
    delete[] patterns_;
    delete[] blocks_;
    delete[] chunks_;
    delete map_;
}

const Palette& Sonic2Level::getPalette(size_t index) const
{
    if (index >= kPaletteCount) {
        throw std::runtime_error("Invalid palette index");
    }

    return palettes_[index];
}

Palette& Sonic2Level::getPalette(size_t index)
{
    if (index >= kPaletteCount) {
        throw std::runtime_error("Invalid palette index");
    }

    return palettes_[index];
}

const Pattern& Sonic2Level::getPattern(size_t index) const
{
    if (index >= patternCount_) {
        throw std::runtime_error("Invalid pattern index");
    }

    return patterns_[index];
}

Pattern& Sonic2Level::getPattern(size_t index)
{
    if (index >= patternCount_) {
        throw std::runtime_error("Invalid pattern index");
    }

    return patterns_[index];
}

const Block& Sonic2Level::getBlock(size_t index) const
{
    if (index >= blockCount_) {
        throw std::runtime_error("Invalid block index " + std::to_string(index));
    }

    return blocks_[index];
}

Block& Sonic2Level::getBlock(size_t index)
{
    if (index >= blockCount_) {
        throw std::runtime_error("Invalid block index " + std::to_string(index));
    }

    return blocks_[index];
}

const Chunk& Sonic2Level::getChunk(size_t index) const
{
    if (index >= chunkCount_) {
        throw std::runtime_error("Invalid chunk index");
    }

    return chunks_[index];
}

Chunk& Sonic2Level::getChunk(size_t index)
{
    if (index >= chunkCount_) {
        throw std::runtime_error("Invalid chunk index");
    }

    return chunks_[index];
}

Map& Sonic2Level::getMap()
{
    return *map_;
}

const std::vector<RingGroup>& Sonic2Level::getRingGroups() const
{
    return ringGroups_;
}

void Sonic2Level::loadPalettes(Rom& rom, uint32_t characterPaletteAddr, uint32_t levelPalettesAddr)
{
    std::vector<char> paletteData(Palette::kPaletteSizeInRom * kPaletteCount);

    {
        auto buffer = rom.readBytes(characterPaletteAddr, Palette::kPaletteSizeInRom);
        copy(buffer.begin(), buffer.end(), paletteData.begin());
    }

    {
        auto buffer = rom.readBytes(levelPalettesAddr, Palette::kPaletteSizeInRom * 3);
        copy(buffer.begin(), buffer.end(), paletteData.begin() + Palette::kPaletteSizeInRom);
    }

    loadPalettes(paletteData);
}

void Sonic2Level::loadPalettes(const std::vector<char>& data)
{
    if (data.size() != Palette::kPaletteSizeInRom * kPaletteCount) {
        throw std::runtime_error("Inconsistent palette data");
    }

    palettes_ = new Palette[kPaletteCount];
    for (size_t i = 0; i < kPaletteCount; i++) {
        palettes_[i].fromSegaFormat(const_cast<char*>(&data[i * Palette::kPaletteSizeInRom]));
    }
}

void Sonic2Level::loadPatterns(Rom& rom, uint32_t patternsAddr)
{
    static constexpr size_t kPatternBufferSize = 0xFFFF;  // 64KB

    // Decompress patterns
    auto& file = rom.getFile();
    file.seek(patternsAddr);
    KosinskiReader reader;
    std::vector<uint8_t> buffer(kPatternBufferSize);
    auto result = reader.decompress(file, buffer.data(), kPatternBufferSize);
    if (!result.first) {
        throw std::runtime_error("Pattern decompression failed");
    }

    buffer.resize(result.second);
    loadPatterns(buffer);
}

void Sonic2Level::loadPatterns(const std::vector<uint8_t>& data)
{
    patternCount_ = data.size() / Pattern::kPatternSizeInRom;
    if (data.size() % Pattern::kPatternSizeInRom != 0) {
        throw std::runtime_error("Inconsistent pattern data");
    }

    patterns_ = new Pattern[patternCount_];
    for (size_t i = 0; i < patternCount_; i++) {
        patterns_[i].fromSegaFormat(const_cast<uint8_t*>(&data[i * Pattern::kPatternSizeInRom]));
    }

    LOG() << "Pattern count: " << patternCount_ << " (" << data.size() << " bytes)";
}

void Sonic2Level::loadBlocks(Rom& rom, uint32_t blocksAddr)
{
    static constexpr size_t kBlockBufferSize = 0xFFFF;  // 64KB

    // Decompress blocks
    auto& file = rom.getFile();
    file.seek(blocksAddr);
    KosinskiReader reader;
    std::vector<uint8_t> buffer(kBlockBufferSize);
    auto result = reader.decompress(file, buffer.data(), kBlockBufferSize);
    if (!result.first) {
        throw std::runtime_error("Block decompression error");
    }

    buffer.resize(result.second);
    loadBlocks(buffer);
}

void Sonic2Level::loadBlocks(const std::vector<uint8_t>& data)
{
    blockCount_ = data.size() / Block::kBlockSizeInRom;
    if (data.size() % Block::kBlockSizeInRom != 0) {
        throw std::runtime_error("Inconsistent block data");
    }

    blocks_ = new Block[blockCount_];
    for (size_t i = 0; i < blockCount_; i++) {
        blocks_[i].fromSegaFormat(const_cast<uint8_t*>(&data[i * Block::kBlockSizeInRom]));
    }

    LOG() << "Block count: " << blockCount_ << " (" << data.size() << " bytes)";
}

void Sonic2Level::loadChunks(Rom& rom, uint32_t chunksAddr)
{
    static constexpr size_t kChunkBufferSize = 0xFFFF;  // 64KB

    // Decompress chunks
    auto& file = rom.getFile();
    file.seek(chunksAddr);
    KosinskiReader reader;
    std::vector<uint8_t> buffer(kChunkBufferSize);
    auto result = reader.decompress(file, buffer.data(), kChunkBufferSize);
    if (!result.first) {
        throw std::runtime_error("Chunk decompression error");
    }

    buffer.resize(result.second);
    loadChunks(buffer);
}

void Sonic2Level::loadChunks(const std::vector<uint8_t>& data)
{
    chunkCount_ = data.size() / Chunk::kChunkSizeInRom;
    if (data.size() % Chunk::kChunkSizeInRom != 0) {
        throw std::runtime_error("Inconsistent chunk data");
    }

    chunks_ = new Chunk[chunkCount_];
    for (size_t i = 0; i < chunkCount_; i++) {
        chunks_[i].fromSegaFormat(const_cast<uint8_t*>(&data[i * Chunk::kChunkSizeInRom]));
    }

    LOG() << "Chunk count: " << chunkCount_ << " (" << data.size() << " bytes)";
}

void Sonic2Level::loadMap(Rom& rom, uint32_t mapAddr)
{
    static constexpr size_t kMapBufferSize = 0xFFFF;  // 64KB

    auto& file = rom.getFile();
    file.seek(mapAddr);
    std::vector<unsigned char> buffer(kMapBufferSize);

    KosinskiReader reader;
    auto result = reader.decompress(file, buffer.data(), kMapBufferSize);
    if (!result.first) {
        throw std::runtime_error("Map decompression error");
    }

    buffer.resize(result.second);
    loadMap(buffer);
}

void Sonic2Level::loadMap(const std::vector<uint8_t>& data)
{
    if (data.size() != kMapLayers * kMapHeight * kMapWidth) {
        throw std::runtime_error("Inconsistent map data");
    }

    map_ = new Map(kMapLayers, kMapWidth, kMapHeight, const_cast<uint8_t*>(data.data()));
}

void Sonic2Level::loadRings(Rom& rom, uint32_t ringsAddr, size_t ringsSize)
{
    const auto bytes = rom.readBytes(ringsAddr, ringsSize);
    std::vector<uint8_t> data(bytes.begin(), bytes.end());
    loadRings(data);
}

void Sonic2Level::loadRings(const std::vector<uint8_t>& data)
{
    ringGroups_ = Sonic2RingLayout::read(data);
    LOG() << "Ring group count: " << ringGroups_.size();
}
