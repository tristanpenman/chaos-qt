#include <algorithm>
#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <QBuffer>

#include "../Block.h"
#include "../Chunk.h"
#include "../KosinskiReader.h"
#include "../KosinskiWriter.h"
#include "../Logger.h"
#include "../Map.h"
#include "../Palette.h"
#include "../Pattern.h"
#include "../Rom.h"

#include "Sonic2Level.h"
#include "Sonic2Rom.h"

#undef LOG
#define LOG Logger("Sonic2Rom")

using namespace std;

static constexpr uint32_t kDefaultLevelLayoutDirAddress = 0x045A80;
static constexpr uint32_t kDefaultRomSize = 0x100000;                    // Size of a standard Sonic 2 ROM (1MB)
static constexpr uint32_t kLevelDataDirectory = 0x42594;                 // Level data pointers (patterns, blocks, chunks)
static constexpr uint32_t kLevelDataDirectoryEntrySize = 12;             // Each pointer is 4 bytes, total of 3 pointers
static constexpr uint32_t kLevelLayoutDirectoryAddressLocation = 0xE46E; // Pointer to directory of layout pointers
static constexpr uint32_t kLevelLayoutDirectorySize = 68;
static constexpr uint32_t kLevelPaletteDirectory = 0x2782;               // Directory of palette pointers
static constexpr uint32_t kLevelSelectAddress = 0x9454;                  // Level select order
static constexpr uint32_t kMaxRingLayoutSize = 0x10000;
static constexpr uint32_t kRingLayoutDirectoryAddressLocation = 0x172D0; // Pointer to directory of ring layout offsets
static constexpr uint32_t kRingLayoutDirectoryEntryCount = 34;
static constexpr uint32_t kSonicTailsPaletteAddress = 0x29E2;            // Default palette used for Sonic and Tails

Sonic2Rom::Sonic2Rom(const shared_ptr<Rom>& rom)
    : rom_(rom)
{

}

bool Sonic2Rom::isCompatible()
{
    const auto name = rom_->readDomesticName();

    return name.find("SONIC THE") != name.npos && name.find("HEDGEHOG 2") != name.npos;
}

const char* Sonic2Rom::getIdentifier() const
{
    return "Sonic2Rom";
}

vector<string> Sonic2Rom::getTitleCards()
{
    return {
        "Emerald Hill Zone - Act 1",
        "Emerald Hill Zone - Act 2",
        "Chemical Plant Zone - Act 1",
        "Chemical Plant Zone - Act 2",
        "Aquatic Ruins Zone - Act 1",
        "Aquatic Ruins Zone - Act 2",
        "Casino Night Zone - Act 1",
        "Casino Night Zone - Act 1",
        "Hill Top Zone - Act 1",
        "Hill Top Zone - Act 2",
        "Mystic Cave Zone - Act 1",
        "Mystic Zone - Act 2",
        "Oil Ocean Zone - Act 1",
        "Oil Ocean Zone - Act 2",
        "Metropolis Zone - Act 1",
        "Metropolis Zone - Act 2",
        "Metropolis Zone - Act 3",
        "Sky Chase Zone - Act 1",
        "Wing Fortress Zone - Act 1",
        "Death Egg Zone - Act 1"
    };
}

shared_ptr<Level> Sonic2Rom::loadLevel(unsigned int levelIdx)
{
    const auto characterPaletteAddr = getCharacterPaletteAddr();
    const auto levelPalettesAddr = getLevelPalettesAddr(levelIdx);
    const auto patternsAddr = getPatternsAddr(levelIdx);
    const auto blocksAddr = getBlocksAddr(levelIdx);
    const auto chunksAddr = getChunksAddr(levelIdx);
    const auto mapAddr = getTilesAddr(levelIdx);
    const auto ringsRegion = getRingsRegion(levelIdx);

    LOG() << "Character palette addr: 0x" << hex << characterPaletteAddr;
    LOG() << "Level palettes addr: 0x" << hex << levelPalettesAddr;
    LOG() << "Patterns addr: 0x" << hex << patternsAddr;
    LOG() << "Blocks addr: 0x" << hex << blocksAddr;
    LOG() << "Chunks addr: 0x" << hex << chunksAddr;
    LOG() << "Map addr: 0x" << hex << mapAddr;
    LOG() << "Rings addr: 0x" << hex << ringsRegion.address;

    return make_shared<Sonic2Level>(*rom_,
                                    characterPaletteAddr,
                                    levelPalettesAddr,
                                    patternsAddr,
                                    blocksAddr,
                                    chunksAddr,
                                    mapAddr,
                                    ringsRegion.address,
                                    ringsRegion.size);
}

bool Sonic2Rom::canRelocateLevels() const
{
    return true;
}

bool Sonic2Rom::canSave() const
{
    return true;
}

bool Sonic2Rom::relocateLevels(bool unsafe)
{
    LOG() << "Relocating levels in " << (unsafe ? "unsafe" : "safe") << " mode";

    // check rom size
    const auto romSize = rom_->getSize();
    if (romSize != kDefaultRomSize && !unsafe) {
        LOG() << "Rom size does not match default; giving up";
        return false;
    }

    // check level layout directory address
    const auto kLevelLayoutDirectoryAddress = rom_->read32BitAddr(kLevelLayoutDirectoryAddressLocation);
    if (kLevelLayoutDirectoryAddress != kDefaultLevelLayoutDirAddress && !unsafe) {
        LOG() << "Level layout directory is not at expected location; giving up";
        return false;
    }

    // relocate levels using level select order
    const auto maxMapSize = 2 * 128 * 16;
    const auto maxMapCount = kLevelLayoutDirectorySize / 2;
    const auto bufferSize = kLevelLayoutDirectorySize + maxMapSize * maxMapCount;
    vector<uint8_t> buffer(bufferSize);

    // setup decompression
    auto& file = rom_->getFile();
    KosinskiReader reader;
    vector<uint8_t> mapBuffer(0xFFFFF);

    uint32_t newLevelOffset = 68;
    for (uint16_t levelIdx = 0; levelIdx < 20; levelIdx++) {
        LOG() << "Relocating level " << levelIdx;

        const uint32_t zoneIdxLoc = kLevelSelectAddress + levelIdx * 2;
        const uint8_t zoneIdx = rom_->readByte(zoneIdxLoc);

        const uint32_t actIdxLoc = zoneIdxLoc + 1;
        const uint8_t actIdx = rom_->readByte(actIdxLoc);

        const uint32_t layoutDirAddr = rom_->read32BitAddr(kLevelLayoutDirectoryAddressLocation);
        const uint32_t levelOffsetLoc = layoutDirAddr + zoneIdx * 4 + actIdx * 2;
        const uint16_t levelOffset = rom_->read16BitAddr(levelOffsetLoc);

        const uint32_t tilesAddr = layoutDirAddr + levelOffset;

        // figure out how many bytes to copy
        file.seek(tilesAddr);
        const auto result = reader.decompress(file, mapBuffer.data(), mapBuffer.size());
        if (!result.first) {
            stringstream ss;
            ss << "Unable to relocate levels; level decompression failed on level " << levelIdx;
            throw std::runtime_error(ss.str());
        }

        // write new location to buffer
        buffer[zoneIdx * 4 + actIdx * 2] = (newLevelOffset >> 8) & 0xFF;
        buffer[zoneIdx * 4 + actIdx * 2 + 1] = newLevelOffset & 0xFF;

        // copy however much data was read by the decompressor
        const auto bytesToCopy = static_cast<size_t>(file.pos()) - tilesAddr;
        LOG() << "Copying " << bytesToCopy << " bytes to 0x" << hex << newLevelOffset;
        file.seek(tilesAddr);
        file.read(reinterpret_cast<char*>(buffer.data() + newLevelOffset),
                  static_cast<qint64>(bytesToCopy));

        newLevelOffset += static_cast<uint32_t>(bytesToCopy);
    }

    rom_->write32BitAddr(static_cast<uint32_t>(romSize), kLevelLayoutDirectoryAddressLocation);
    file.seek(romSize);
    file.write(reinterpret_cast<const char*>(buffer.data()), static_cast<qint64>(bufferSize));

    // write new rom size
    const auto newAddrRange = static_cast<uint32_t>(romSize + bufferSize - 1);
    LOG() << "Writing new address range: " << newAddrRange;
    rom_->writeSize(newAddrRange);

    // write new checksum
    const auto checksum = rom_->calculateChecksum();
    LOG() << "Writing new checksum: " << checksum;
    rom_->writeChecksum(checksum);

    return true;
}

bool Sonic2Rom::save(unsigned int levelIdx, Level& level)
{
    auto tilesAddr = getTilesAddr(levelIdx);
    auto characterPaletteAddr = getCharacterPaletteAddr();
    auto levelPalettesAddr = getLevelPalettesAddr(levelIdx);
    auto patternsAddr = getPatternsAddr(levelIdx);
    auto blocksAddr = getBlocksAddr(levelIdx);
    auto chunksAddr = getChunksAddr(levelIdx);
    optional<size_t> mapLimit;
    optional<size_t> patternLimit;
    optional<size_t> blockLimit;
    optional<size_t> chunkLimit;
    auto& file = rom_->getFile();

    // if levels have not been relocated, check how much space we have
    const auto kLevelLayoutDirectoryAddress = rom_->read32BitAddr(kLevelLayoutDirectoryAddressLocation);
    if (kLevelLayoutDirectoryAddress == kDefaultLevelLayoutDirAddress) {
        LOG() << "Levels have not been relocated; checking how much space is available...";

        std::vector<uint8_t> buffer(0xFFFF);
        KosinskiReader reader;
        file.seek(tilesAddr);
        auto result = reader.decompress(file, buffer.data(), buffer.size());
        if (!result.first) {
            LOG() << "Failed to fully extract existing level at location 0x" << hex << tilesAddr;
            return false;
        }

        mapLimit = size_t(file.pos()) - tilesAddr;
        LOG() << "Total space available is " << *mapLimit << " bytes";
    }

    {
        char paletteData[Palette::kPaletteSizeInRom];
        level.getPalette(0).toSegaFormat(paletteData);
        file.seek(characterPaletteAddr);
        file.write(paletteData, Palette::kPaletteSizeInRom);

        for (size_t i = 1; i < level.getPaletteCount(); i++) {
            level.getPalette(i).toSegaFormat(paletteData);
            file.seek(levelPalettesAddr + static_cast<uint32_t>((i - 1) * Palette::kPaletteSizeInRom));
            file.write(paletteData, Palette::kPaletteSizeInRom);
        }
    }

    {
        std::vector<uint8_t> buffer(0xFFFF);
        KosinskiReader reader;
        file.seek(patternsAddr);
        auto result = reader.decompress(file, buffer.data(), buffer.size());
        if (!result.first) {
            LOG() << "Failed to fully extract existing patterns at location 0x" << hex << patternsAddr;
            return false;
        }

        patternLimit = size_t(file.pos()) - patternsAddr;
        LOG() << "Total pattern space available is " << *patternLimit << " bytes";
    }

    {
        std::vector<uint8_t> buffer(0xFFFF);
        KosinskiReader reader;
        file.seek(blocksAddr);
        auto result = reader.decompress(file, buffer.data(), buffer.size());
        if (!result.first) {
            LOG() << "Failed to fully extract existing blocks at location 0x" << hex << blocksAddr;
            return false;
        }

        blockLimit = size_t(file.pos()) - blocksAddr;
        LOG() << "Total block space available is " << *blockLimit << " bytes";
    }

    {
        std::vector<uint8_t> buffer(0xFFFF);
        KosinskiReader reader;
        file.seek(chunksAddr);
        auto result = reader.decompress(file, buffer.data(), buffer.size());
        if (!result.first) {
            LOG() << "Failed to fully extract existing chunks at location 0x" << hex << chunksAddr;
            return false;
        }

        chunkLimit = size_t(file.pos()) - chunksAddr;
        LOG() << "Total chunk space available is " << *chunkLimit << " bytes";
    }

    vector<uint8_t> patternData(level.getPatternCount() * Pattern::kPatternSizeInRom);
    for (size_t i = 0; i < level.getPatternCount(); i++) {
        level.getPattern(i).toSegaFormat(&patternData[i * Pattern::kPatternSizeInRom]);
    }

    QByteArray compressedPatterns;
    QBuffer patternBuffer(&compressedPatterns);
    patternBuffer.open(QIODevice::WriteOnly);
    KosinskiWriter patternWriter;
    auto patternResult = patternWriter.compress(patternBuffer, patternData.data(), patternData.size(), patternLimit);
    if (!patternResult.first) {
        LOG() << "Failed to write pattern data at location 0x" << hex << patternsAddr << "; not enough space";
        return false;
    }

    vector<uint8_t> blockData(level.getBlockCount() * Block::kBlockSizeInRom);
    for (size_t i = 0; i < level.getBlockCount(); i++) {
        level.getBlock(i).toSegaFormat(&blockData[i * Block::kBlockSizeInRom]);
    }

    QByteArray compressedBlocks;
    QBuffer blockBuffer(&compressedBlocks);
    blockBuffer.open(QIODevice::WriteOnly);
    KosinskiWriter blockWriter;
    auto blockResult = blockWriter.compress(blockBuffer, blockData.data(), blockData.size(), blockLimit);
    if (!blockResult.first) {
        LOG() << "Failed to write block data at location 0x" << hex << blocksAddr << "; not enough space";
        return false;
    }

    vector<uint8_t> chunkData(level.getChunkCount() * Chunk::kChunkSizeInRom);
    for (size_t i = 0; i < level.getChunkCount(); i++) {
        level.getChunk(i).toSegaFormat(&chunkData[i * Chunk::kChunkSizeInRom]);
    }

    QByteArray compressedChunks;
    QBuffer chunkBuffer(&compressedChunks);
    chunkBuffer.open(QIODevice::WriteOnly);
    KosinskiWriter chunkWriter;
    auto chunkResult = chunkWriter.compress(chunkBuffer, chunkData.data(), chunkData.size(), chunkLimit);
    if (!chunkResult.first) {
        LOG() << "Failed to write chunk data at location 0x" << hex << chunksAddr << "; not enough space";
        return false;
    }

    auto& map = level.getMap();
    auto data = map.getData();
    auto dataSize = map.getHeight() * map.getWidth() * map.getLayerCount();

    KosinskiWriter writer;
    file.seek(tilesAddr);
    auto result = writer.compress(file, data, dataSize, mapLimit);
    if (!result.first) {
        LOG() << "Failed to write level data at location 0x" << hex << tilesAddr << "; not enough space";
        return false;
    }

    LOG() << "Wrote " << result.second << " bytes to location 0x" << hex << tilesAddr;

    file.seek(patternsAddr);
    file.write(compressedPatterns);
    LOG() << "Wrote " << patternResult.second << " pattern bytes to location 0x" << hex << patternsAddr;

    file.seek(blocksAddr);
    file.write(compressedBlocks);
    LOG() << "Wrote " << blockResult.second << " block bytes to location 0x" << hex << blocksAddr;

    file.seek(chunksAddr);
    file.write(compressedChunks);
    LOG() << "Wrote " << chunkResult.second << " chunk bytes to location 0x" << hex << chunksAddr;

    LOG() << "Updating checksum...";
    rom_->writeChecksum(rom_->calculateChecksum());

    return true;
}

uint32_t Sonic2Rom::getDataAddress(unsigned int levelIdx, unsigned int entryOffset)
{
    const uint32_t levelDataIdxLoc = kLevelSelectAddress + levelIdx * 2;
    const uint8_t levelDataIdx = rom_->readByte(levelDataIdxLoc);

    const uint32_t dataAddrLoc = kLevelDataDirectory +
        levelDataIdx * kLevelDataDirectoryEntrySize +
        entryOffset;

    return rom_->read32BitAddr(dataAddrLoc);
}

uint32_t Sonic2Rom::getCharacterPaletteAddr()
{
    return kSonicTailsPaletteAddress;
}

uint32_t Sonic2Rom::getLevelPalettesAddr(unsigned int levelIdx)
{
    const uint32_t dataAddr = getDataAddress(levelIdx, 8);
    const uint32_t paletteIndex = dataAddr >> 24;
    const uint32_t paletteAddrLoc = kLevelPaletteDirectory + paletteIndex * 8;

    return rom_->read32BitAddr(paletteAddrLoc);
}

uint32_t Sonic2Rom::getChunksAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 8) & 0xFFFFFF;
}

uint32_t Sonic2Rom::getBlocksAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 4) & 0xFFFFFF;
}

uint32_t Sonic2Rom::getPatternsAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 0) & 0xFFFFFF;
}

uint32_t Sonic2Rom::getTilesAddr(unsigned int levelIdx)
{
    const uint32_t zoneIdxLoc = kLevelSelectAddress + levelIdx * 2;
    const uint8_t zoneIdx = rom_->readByte(zoneIdxLoc);

    const uint32_t actIdxLoc = zoneIdxLoc + 1;
    const uint8_t actIdx = rom_->readByte(actIdxLoc);

    const uint32_t kLevelLayoutDirectoryAddress = rom_->read32BitAddr(kLevelLayoutDirectoryAddressLocation);
    const uint32_t levelOffsetLoc = kLevelLayoutDirectoryAddress + zoneIdx * 4 + actIdx * 2;
    const uint16_t levelOffset = rom_->read16BitAddr(levelOffsetLoc);

    return kLevelLayoutDirectoryAddress + levelOffset;
}

Sonic2Rom::DataRegion Sonic2Rom::getRingsRegion(uint32_t levelIdx)
{
    const uint32_t levelSelectEntry = kLevelSelectAddress + levelIdx * 2;
    const uint32_t zoneIdx = rom_->readByte(levelSelectEntry);
    const uint32_t actIdx = rom_->readByte(levelSelectEntry + 1);
    const uint32_t entryIdx = zoneIdx * 2 + actIdx;
    if (entryIdx >= kRingLayoutDirectoryEntryCount) {
        throw runtime_error("Invalid Sonic 2 ring layout index");
    }

    const uint32_t directoryAddr = rom_->read32BitAddr(kRingLayoutDirectoryAddressLocation);
    const uint16_t selectedOffset = rom_->read16BitAddr(directoryAddr + entryIdx * 2);
    const uint32_t ringsAddr = directoryAddr + selectedOffset;
    if (static_cast<size_t>(directoryAddr) + kRingLayoutDirectoryEntryCount * 2 > rom_->getSize() ||
        ringsAddr >= rom_->getSize()) {
        throw runtime_error("Sonic 2 ring layout directory is outside the ROM");
    }

    uint32_t endAddr = static_cast<uint32_t>(min(
        rom_->getSize(),
        static_cast<size_t>(ringsAddr) + kMaxRingLayoutSize));
    for (uint32_t i = 0; i < kRingLayoutDirectoryEntryCount; i++) {
        const uint16_t offset = rom_->read16BitAddr(directoryAddr + i * 2);
        const uint32_t candidate = directoryAddr + offset;
        if (candidate > ringsAddr && candidate < endAddr) {
            endAddr = candidate;
        }
    }

    return {ringsAddr, endAddr - ringsAddr};
}
