#include "Sonic3Rom.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "../Logger.h"
#include "../Rom.h"

#include "Sonic3Level.h"

#undef LOG
#define LOG Logger("Sonic3Rom")

namespace {

constexpr uint32_t kLevelDataDirectory = 0x5AB0C;          // Level data pointers (patterns, blocks, chunks)
constexpr uint32_t kLevelDataDirectoryEntrySize = 24;      // Each pointer is 4 bytes, total of 3 pointers
constexpr uint32_t kLevelLayoutDirectoryAddress = 0x81360; // Layout pointers are found here
constexpr uint32_t kLevelPaletteDirectory = 0x8BF54;       // Directory of palette pointers
constexpr uint32_t kLevelSelectIndex = 0x6A8E;             // Level select order

}  // namespace

Sonic3Rom::Sonic3Rom(const std::shared_ptr<Rom>& rom)
    : rom_(rom)
{

}

bool Sonic3Rom::isCompatible()
{
    const auto name = rom_->readDomesticName();

    return name.find("SONIC THE") != name.npos && name.find("HEDGEHOG 3") != name.npos;
}

const char* Sonic3Rom::getIdentifier() const
{
    return "Sonic3Rom";
}

std::vector<std::string> Sonic3Rom::getTitleCards()
{
    return {
        "Angel Island Zone - Act 1",
        "Angel Island Zone - Act 2",
        "Hydrocity Zone - Act 1",
        "Hydrocity Zone - Act 2",
        "Marble Garden Zone - Act 1",
        "Marble Garden Zone - Act 2",
        "Carnival Night Zone - Act 1",
        "Carnival Night Zone - Act 1",
        "--",
        "--",
        "Ice Cap Zone - Act 1",
        "Ice Cap Zone - Act 2",
        "Launch Base Zone - Act 1",
        "Launch Base Zone - Act 2"
    };
}

std::shared_ptr<Level> Sonic3Rom::loadLevel(unsigned int levelIdx)
{
    const auto characterPaletteAddr = getCharacterPaletteAddr();
    const auto levelPalettesAddr = getLevelPalettesAddr(levelIdx);
    const auto patternsAddr = getPatternsAddr(levelIdx);
    const auto extendedPatternsAddr = getExtendedPatternsAddr(levelIdx);
    const auto blocksAddr = getBlocksAddr(levelIdx);
    const auto extendedBlocksAddr = getExtendedBlocksAddr(levelIdx);
    const auto chunksAddr = getChunksAddr(levelIdx);
    const auto extendedChunksAddr = getExtendedChunksAddr(levelIdx);
    const auto mapAddr = getTilesAddr(levelIdx);

    LOG() << "Character palette addr: 0x" << std::hex << characterPaletteAddr;
    LOG() << "Level palettes addr: 0x" << std::hex << levelPalettesAddr;
    LOG() << "Patterns addr: 0x" << std::hex << patternsAddr;
    LOG() << "Extended patterns addr: 0x" << std::hex << extendedPatternsAddr;
    LOG() << "Blocks addr: 0x" << std::hex << blocksAddr;
    LOG() << "Extended blocks addr: 0x" << std::hex << extendedBlocksAddr;
    LOG() << "Chunks addr: 0x" << std::hex << chunksAddr;
    LOG() << "Extended chunks addr: 0x" << std::hex << extendedChunksAddr;
    LOG() << "Map addr: 0x" << std::hex << mapAddr;

    return std::make_shared<Sonic3Level>(*rom_,
                                    characterPaletteAddr,
                                    levelPalettesAddr,
                                    patternsAddr,
                                    extendedPatternsAddr,
                                    blocksAddr,
                                    extendedBlocksAddr,
                                    chunksAddr,
                                    extendedChunksAddr,
                                    mapAddr);
}

bool Sonic3Rom::canRelocateLevels() const
{
    return false;
}

bool Sonic3Rom::canSave() const
{
    return false;
}

bool Sonic3Rom::relocateLevels(bool)
{
    throw std::runtime_error("Not implemented");
}

bool Sonic3Rom::save(unsigned int, Level&)
{
    return false;
}

uint32_t Sonic3Rom::getDataAddress(unsigned int levelIdx, unsigned int entryOffset)
{
    const uint32_t zoneIndexLoc = kLevelSelectIndex + levelIdx * 2;
    const uint32_t zoneIndex = rom_->readByte(zoneIndexLoc);

    const uint32_t actIndexLoc = zoneIndexLoc + 1;
    const uint32_t actIndex = rom_->readByte(actIndexLoc);

    const uint32_t dataAddrLoc = kLevelDataDirectory +
        zoneIndex * kLevelDataDirectoryEntrySize * 2 +
        actIndex * kLevelDataDirectoryEntrySize +
        entryOffset;

    return rom_->read32BitAddr(dataAddrLoc);
}

uint32_t Sonic3Rom::getCharacterPaletteAddr()
{
    return 0x8C234;
}

uint32_t Sonic3Rom::getLevelPalettesAddr(unsigned int levelIdx)
{
    const uint32_t dataAddr = getDataAddress(levelIdx, 8);
    const uint32_t paletteIdx = dataAddr >> 24;
    const uint32_t paletteAddrLoc = kLevelPaletteDirectory + paletteIdx * 8;

    return rom_->read32BitAddr(paletteAddrLoc);
}

uint32_t Sonic3Rom::getChunksAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 16) & 0xFFFFFF;
}

uint32_t Sonic3Rom::getBlocksAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 8) & 0xFFFFFF;
}

uint32_t Sonic3Rom::getPatternsAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 0) & 0xFFFFFF;
}

uint32_t Sonic3Rom::getTilesAddr(unsigned int levelIdx)
{
    const uint32_t tilesAddrLoc = kLevelLayoutDirectoryAddress + levelIdx * 4;

    return rom_->read32BitAddr(tilesAddrLoc);
}

uint32_t Sonic3Rom::getExtendedChunksAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 20) & 0xFFFFFF;
}

uint32_t Sonic3Rom::getExtendedBlocksAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 12) & 0xFFFFFF;
}

uint32_t Sonic3Rom::getExtendedPatternsAddr(unsigned int levelIdx)
{
    return getDataAddress(levelIdx, 4) & 0xFFFFFF;
}
