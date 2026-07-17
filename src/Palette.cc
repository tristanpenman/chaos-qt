#include <cstring>
#include <stdexcept>

#include "Palette.h"

using namespace std;

Palette::Palette()
{
    memset(colors_, 0, sizeof(Color) * PALETTE_SIZE);
}

void Palette::fromSegaFormat(char bytes[PALETTE_SIZE_IN_ROM])
{
    for (int index = 0; index < PALETTE_SIZE; index++) {
        colors_[index].fromSegaFormat(&bytes[index * BYTES_PER_COLOR]);
    }
}

void Palette::toSegaFormat(char bytes[PALETTE_SIZE_IN_ROM]) const
{
    for (int index = 0; index < PALETTE_SIZE; index++) {
        colors_[index].toSegaFormat(&bytes[index * BYTES_PER_COLOR]);
    }
}

const Palette::Color& Palette::getColor(size_t index) const
{
    if (index >= PALETTE_SIZE) {
        throw runtime_error("Invalid palette index");
    }

    return colors_[index];
}

void Palette::setColor(size_t index, const Color& color)
{
    if (index >= PALETTE_SIZE) {
        throw runtime_error("Invalid palette index");
    }

    memcpy(&colors_[index], &color, sizeof(Color));
}

void Palette::Color::fromSegaFormat(char bytes[BYTES_PER_COLOR])
{
    r = static_cast<uint8_t>((bytes[1] & 0x0F) * 0x10);
    g = static_cast<uint8_t>(bytes[1] & 0xF0);
    b = static_cast<uint8_t>(bytes[0] * 0x10);
}

void Palette::Color::toSegaFormat(char bytes[BYTES_PER_COLOR]) const
{
    bytes[0] = static_cast<char>((b / 0x10) & 0x0F);
    bytes[1] = static_cast<char>((g & 0xF0) | ((r / 0x10) & 0x0F));
}
