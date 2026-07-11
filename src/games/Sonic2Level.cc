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

#include "Sonic2Level.h"
#include "Sonic2RingLayout.h"

#undef LOG
#define LOG Logger("Sonic2Level")

static constexpr uint8_t MAP_LAYERS = 2;
static constexpr uint8_t MAP_HEIGHT = 16;
static constexpr uint8_t MAP_WIDTH = 128;

using namespace std;

Sonic2Level::Sonic2Level(Rom& rom,
                         uint32_t sonicPaletteAddr,
                         uint32_t levelPalettesAddr,
                         uint32_t patternsAddr,
                         uint32_t blocksAddr,
                         uint32_t chunksAddr,
                         uint32_t mapAddr,
                         uint32_t ringsAddr,
                         size_t ringsSize)
    : m_palettes(nullptr)
    , m_patterns(nullptr)
    , m_blocks(nullptr)
    , m_chunks(nullptr)
    , m_map(nullptr)
    , m_patternCount(0)
    , m_blockCount(0)
    , m_chunkCount(0)
{
    loadPalettes(rom, sonicPaletteAddr, levelPalettesAddr);
    loadPatterns(rom, patternsAddr);
    loadBlocks(rom, blocksAddr);
    loadChunks(rom, chunksAddr);
    loadMap(rom, mapAddr);
    loadRings(rom, ringsAddr, ringsSize);
}

Sonic2Level::Sonic2Level(const vector<char>& paletteData,
                         const vector<uint8_t>& patternData,
                         const vector<uint8_t>& blockData,
                         const vector<uint8_t>& chunkData,
                         const vector<uint8_t>& mapData,
                         const vector<uint8_t>& ringData)
    : m_palettes(nullptr)
    , m_patterns(nullptr)
    , m_blocks(nullptr)
    , m_chunks(nullptr)
    , m_map(nullptr)
    , m_patternCount(0)
    , m_blockCount(0)
    , m_chunkCount(0)
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
    delete[] m_palettes;
    delete[] m_patterns;
    delete[] m_blocks;
    delete[] m_chunks;
    delete m_map;
}

const Palette& Sonic2Level::getPalette(size_t index) const
{
    if (index >= PALETTE_COUNT) {
        throw runtime_error("Invalid palette index");
    }

    return m_palettes[index];
}

Palette& Sonic2Level::getPalette(size_t index)
{
    if (index >= PALETTE_COUNT) {
        throw runtime_error("Invalid palette index");
    }

    return m_palettes[index];
}

const Pattern& Sonic2Level::getPattern(size_t index) const
{
    if (index >= m_patternCount) {
        throw runtime_error("Invalid pattern index");
    }

    return m_patterns[index];
}

Pattern& Sonic2Level::getPattern(size_t index)
{
    if (index >= m_patternCount) {
        throw runtime_error("Invalid pattern index");
    }

    return m_patterns[index];
}

const Block& Sonic2Level::getBlock(size_t index) const
{
    if (index >= m_blockCount) {
        throw runtime_error("Invalid block index " + std::to_string(index));
    }

    return m_blocks[index];
}

Block& Sonic2Level::getBlock(size_t index)
{
    if (index >= m_blockCount) {
        throw runtime_error("Invalid block index " + std::to_string(index));
    }

    return m_blocks[index];
}

const Chunk& Sonic2Level::getChunk(size_t index) const
{
    if (index >= m_chunkCount) {
        throw runtime_error("Invalid chunk index");
    }

    return m_chunks[index];
}

Chunk& Sonic2Level::getChunk(size_t index)
{
    if (index >= m_chunkCount) {
        throw runtime_error("Invalid chunk index");
    }

    return m_chunks[index];
}

Map& Sonic2Level::getMap()
{
    return *m_map;
}

const vector<RingGroup>& Sonic2Level::getRingGroups() const
{
    return m_ringGroups;
}

void Sonic2Level::loadPalettes(Rom& rom, uint32_t characterPaletteAddr, uint32_t levelPalettesAddr)
{
    vector<char> paletteData(Palette::PALETTE_SIZE_IN_ROM * PALETTE_COUNT);

    {
        auto buffer = rom.readBytes(characterPaletteAddr, Palette::PALETTE_SIZE_IN_ROM);
        copy(buffer.begin(), buffer.end(), paletteData.begin());
    }

    {
        auto buffer = rom.readBytes(levelPalettesAddr, Palette::PALETTE_SIZE_IN_ROM * 3);
        copy(buffer.begin(), buffer.end(), paletteData.begin() + Palette::PALETTE_SIZE_IN_ROM);
    }

    loadPalettes(paletteData);
}

void Sonic2Level::loadPalettes(const vector<char>& data)
{
    if (data.size() != Palette::PALETTE_SIZE_IN_ROM * PALETTE_COUNT) {
        throw runtime_error("Inconsistent palette data");
    }

    m_palettes = new Palette[PALETTE_COUNT];
    for (size_t i = 0; i < PALETTE_COUNT; i++) {
        m_palettes[i].fromSegaFormat(const_cast<char*>(&data[i * Palette::PALETTE_SIZE_IN_ROM]));
    }
}

void Sonic2Level::loadPatterns(Rom& rom, uint32_t patternsAddr)
{
    static constexpr size_t PATTERN_BUFFER_SIZE = 0xFFFF;  // 64KB

    // decompress patterns
    auto& file = rom.getFile();
    file.seek(patternsAddr);
    KosinskiReader reader;
    std::vector<uint8_t> buffer(PATTERN_BUFFER_SIZE);
    auto result = reader.decompress(file, buffer.data(), PATTERN_BUFFER_SIZE);
    if (!result.first) {
        throw runtime_error("Pattern decompression failed");
    }

    buffer.resize(result.second);
    loadPatterns(buffer);
}

void Sonic2Level::loadPatterns(const vector<uint8_t>& data)
{
    // check data
    m_patternCount = data.size() / Pattern::PATTERN_SIZE_IN_ROM;
    if (data.size() % Pattern::PATTERN_SIZE_IN_ROM != 0) {
        throw runtime_error("Inconsistent pattern data");
    }

    // convert pattern data
    m_patterns = new Pattern[m_patternCount];
    for (size_t i = 0; i < m_patternCount; i++) {
        m_patterns[i].fromSegaFormat(const_cast<uint8_t*>(&data[i * Pattern::PATTERN_SIZE_IN_ROM]));
    }

    LOG() << "Pattern count: " << m_patternCount << " (" << data.size() << " bytes)";
}

void Sonic2Level::loadBlocks(Rom& rom, uint32_t blocksAddr)
{
    static constexpr size_t BLOCK_BUFFER_SIZE = 0xFFFF;  // 64KB

    // decompress blocks
    auto& file = rom.getFile();
    file.seek(blocksAddr);
    KosinskiReader reader;
    vector<uint8_t> buffer(BLOCK_BUFFER_SIZE);
    auto result = reader.decompress(file, buffer.data(), BLOCK_BUFFER_SIZE);
    if (!result.first) {
        throw runtime_error("Block decompression error");
    }

    buffer.resize(result.second);
    loadBlocks(buffer);
}

void Sonic2Level::loadBlocks(const vector<uint8_t>& data)
{
    // check data
    m_blockCount = data.size() / Block::BLOCK_SIZE_IN_ROM;
    if (data.size() % Block::BLOCK_SIZE_IN_ROM != 0) {
        throw runtime_error("Inconsistent block data");
    }

    // convert block data
    m_blocks = new Block[m_blockCount];
    for (size_t i = 0; i < m_blockCount; i++) {
        m_blocks[i].fromSegaFormat(const_cast<uint8_t*>(&data[i * Block::BLOCK_SIZE_IN_ROM]));
    }

    LOG() << "Block count: " << m_blockCount << " (" << data.size() << " bytes)";
}

void Sonic2Level::loadChunks(Rom& rom, uint32_t chunksAddr)
{
    static constexpr size_t CHUNK_BUFFER_SIZE = 0xFFFF;  // 64KB

    // decompress chunks
    auto& file = rom.getFile();
    file.seek(chunksAddr);
    KosinskiReader reader;
    vector<uint8_t> buffer(CHUNK_BUFFER_SIZE);
    auto result = reader.decompress(file, buffer.data(), CHUNK_BUFFER_SIZE);
    if (!result.first) {
        throw runtime_error("Chunk decompression error");
    }

    buffer.resize(result.second);
    loadChunks(buffer);
}

void Sonic2Level::loadChunks(const vector<uint8_t>& data)
{
    // check data
    m_chunkCount = data.size() / Chunk::CHUNK_SIZE_IN_ROM;
    if (data.size() % Chunk::CHUNK_SIZE_IN_ROM != 0) {
        throw runtime_error("Inconsistent chunk data");
    }

    m_chunks = new Chunk[m_chunkCount];
    for (size_t i = 0; i < m_chunkCount; i++) {
        m_chunks[i].fromSegaFormat(const_cast<uint8_t*>(&data[i * Chunk::CHUNK_SIZE_IN_ROM]));
    }

    LOG() << "Chunk count: " << m_chunkCount << " (" << data.size() << " bytes)";
}

void Sonic2Level::loadMap(Rom& rom, uint32_t mapAddr)
{
    static constexpr size_t MAP_BUFFER_SIZE = 0xFFFF;  // 64KB

    auto& file = rom.getFile();
    file.seek(mapAddr);
    vector<unsigned char> buffer(MAP_BUFFER_SIZE);

    KosinskiReader reader;
    auto result = reader.decompress(file, buffer.data(), MAP_BUFFER_SIZE);
    if (!result.first) {
        throw runtime_error("Map decompression error");
    }

    buffer.resize(result.second);
    loadMap(buffer);
}

void Sonic2Level::loadMap(const vector<uint8_t>& data)
{
    // check data
    if (data.size() != MAP_LAYERS * MAP_HEIGHT * MAP_WIDTH) {
        throw runtime_error("Inconsistent map data");
    }

    m_map = new Map(MAP_LAYERS, MAP_WIDTH, MAP_HEIGHT, const_cast<uint8_t*>(data.data()));
}

void Sonic2Level::loadRings(Rom& rom, uint32_t ringsAddr, size_t ringsSize)
{
    const auto bytes = rom.readBytes(ringsAddr, ringsSize);
    vector<uint8_t> data(bytes.begin(), bytes.end());
    loadRings(data);
}

void Sonic2Level::loadRings(const vector<uint8_t>& data)
{
    m_ringGroups = Sonic2RingLayout::read(data);
    LOG() << "Ring group count: " << m_ringGroups.size();
}
