#include <cstring>

#include "Pattern.h"

using namespace std;

Pattern::Pattern()
{
    memset(pixels_, 0, kPatternSizeInMemory);
}

void Pattern::fromSegaFormat(uint8_t buffer[kPatternSizeInRom])
{
    uint8_t bufferPos = 0;
    for (uint8_t row = 0; row < kPatternHeight; row++) {
        for (uint8_t col = 0; col < kPatternWidth; col += 2) {
            pixels_[row * kPatternWidth + col] = (buffer[bufferPos] >> 4) & 0x0F;
            pixels_[row * kPatternWidth + col + 1] = buffer[bufferPos] & 0x0F;
            bufferPos++;
        }
    }
}

void Pattern::toSegaFormat(uint8_t buffer[kPatternSizeInRom]) const
{
    uint8_t bufferPos = 0;
    for (uint8_t row = 0; row < kPatternHeight; row++) {
        for (uint8_t col = 0; col < kPatternWidth; col += 2) {
            buffer[bufferPos] = static_cast<uint8_t>((pixels_[row * kPatternWidth + col] << 4)
                                                     | (pixels_[row * kPatternWidth + col + 1] & 0x0F));
            bufferPos++;
        }
    }
}

uint8_t Pattern::getPixel(uint8_t x, uint8_t y) const
{
    return pixels_[y * kPatternWidth + x];
}

void Pattern::setPixel(uint8_t x, uint8_t y, uint8_t value)
{
    pixels_[y * kPatternWidth + x] = value;
}
