#include <cstring>

#include "Pattern.h"

using namespace std;

Pattern::Pattern()
{
    memset(pixels_, 0, PATTERN_SIZE_IN_MEM);
}

void Pattern::fromSegaFormat(uint8_t buffer[PATTERN_SIZE_IN_ROM])
{
    uint8_t bufferPos = 0;
    for (uint8_t row = 0; row < PATTERN_HEIGHT; row++) {
        for (uint8_t col = 0; col < PATTERN_WIDTH; col += 2) {
            pixels_[row * PATTERN_WIDTH + col] = (buffer[bufferPos] >> 4) & 0x0F;
            pixels_[row * PATTERN_WIDTH + col + 1] = buffer[bufferPos] & 0x0F;
            bufferPos++;
        }
    }
}

void Pattern::toSegaFormat(uint8_t buffer[PATTERN_SIZE_IN_ROM]) const
{
    uint8_t bufferPos = 0;
    for (uint8_t row = 0; row < PATTERN_HEIGHT; row++) {
        for (uint8_t col = 0; col < PATTERN_WIDTH; col += 2) {
            buffer[bufferPos] = static_cast<uint8_t>((pixels_[row * PATTERN_WIDTH + col] << 4)
                                                     | (pixels_[row * PATTERN_WIDTH + col + 1] & 0x0F));
            bufferPos++;
        }
    }
}

uint8_t Pattern::getPixel(uint8_t x, uint8_t y) const
{
    return pixels_[y * PATTERN_WIDTH + x];
}

void Pattern::setPixel(uint8_t x, uint8_t y, uint8_t value)
{
    pixels_[y * PATTERN_WIDTH + x] = value;
}
