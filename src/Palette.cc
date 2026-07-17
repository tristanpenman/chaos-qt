#include "Palette.h"

#include <cstring>
#include <stdexcept>



Palette::Palette()
{
    memset(colors_, 0, sizeof(Color) * kPaletteSize);
}

void Palette::fromSegaFormat(char bytes[kPaletteSizeInRom])
{
    for (int index = 0; index < kPaletteSize; index++) {
        colors_[index].fromSegaFormat(&bytes[index * kBytesPerColor]);
    }
}

void Palette::toSegaFormat(char bytes[kPaletteSizeInRom]) const
{
    for (int index = 0; index < kPaletteSize; index++) {
        colors_[index].toSegaFormat(&bytes[index * kBytesPerColor]);
    }
}

const Palette::Color& Palette::getColor(size_t index) const
{
    if (index >= kPaletteSize) {
        throw std::runtime_error("Invalid palette index");
    }

    return colors_[index];
}

void Palette::setColor(size_t index, const Color& color)
{
    if (index >= kPaletteSize) {
        throw std::runtime_error("Invalid palette index");
    }

    memcpy(&colors_[index], &color, sizeof(Color));
}

void Palette::Color::fromSegaFormat(char bytes[kBytesPerColor])
{
    r = static_cast<uint8_t>((bytes[1] & 0x0F) * 0x10);
    g = static_cast<uint8_t>(bytes[1] & 0xF0);
    b = static_cast<uint8_t>(bytes[0] * 0x10);
}

void Palette::Color::toSegaFormat(char bytes[kBytesPerColor]) const
{
    bytes[0] = static_cast<char>((b / 0x10) & 0x0F);
    bytes[1] = static_cast<char>((g & 0xF0) | ((r / 0x10) & 0x0F));
}
