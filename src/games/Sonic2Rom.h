#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Game.h"

class Level;
class Rom;

class Sonic2Rom : public Game
{
public:
    Sonic2Rom(const std::shared_ptr<Rom>& rom);

    bool isCompatible() override;
    const char* getIdentifier() const override;
    std::vector<std::string> getTitleCards() override;
    std::shared_ptr<Level> loadLevel(unsigned int levelIdx) override;
    bool canRelocateLevels() const override;
    bool canSave() const override;
    bool relocateLevels(bool unsafe) override;
    bool save(unsigned int levelIdx, Level&) override;

private:
    struct DataRegion
    {
        uint32_t address;
        size_t size;
    };

    uint32_t getDataAddress(uint32_t levelIdx, uint32_t entryOffset);

    // uncompressed
    uint32_t getCharacterPaletteAddr();
    uint32_t getLevelPalettesAddr(uint32_t levelIdx);

    // Kosinski compressed
    uint32_t getChunksAddr(uint32_t levelIdx);
    uint32_t getBlocksAddr(uint32_t levelIdx);
    uint32_t getPatternsAddr(uint32_t levelIdx);
    uint32_t getTilesAddr(uint32_t levelIdx);
    DataRegion getRingsRegion(uint32_t levelIdx);

    std::shared_ptr<Rom> rom_;
};
