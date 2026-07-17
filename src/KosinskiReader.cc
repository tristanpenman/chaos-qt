#include "KosinskiReader.h"

#include <sstream>



KosinskiReader::KosinskiReader()
    : bitfield_(0)
    , bitcount_(0)
{

}

uint8_t KosinskiReader::getBit(QIODevice& file)
{
    const uint8_t bit = static_cast<uint8_t>(bitfield_) & 1;

    bitfield_ >>= 1;
    bitcount_--;

    // Ensure that there are more bits to read
    if (bitcount_ == 0) {
        loadBitfield(file);
        if (file.atEnd()) {
            std::stringstream ss("Unexpected end of file at offset ");
            ss << file.pos();
            throw std::runtime_error(ss.str());
        }
    }

    return bit;
}

void KosinskiReader::loadBitfield(QIODevice& file)
{
    bitfield_  = static_cast<uint16_t>(readByte(file));
    bitfield_ |= static_cast<uint16_t>(readByte(file)) << 8;

    bitcount_ = 16;
}

uint8_t KosinskiReader::readByte(QIODevice& file)
{
    const auto offset = file.pos();
    char byte = 0;
    const auto bytesRead = file.read(&byte, 1);

    if (bytesRead != 1) {
        std::stringstream ss;
        ss << "Unexpected end of file at offset ";
        ss << offset;
        throw std::runtime_error(ss.str());
    }

    return static_cast<uint8_t>(byte);
}

KosinskiReader::Result KosinskiReader::decompress(QIODevice& file, uint8_t buffer[], size_t bufferSize)
{
    if (buffer == nullptr) {
        return Result(false, 0);
    }

    uint16_t pos = 0;
    uint16_t count = 0;
    int16_t offset = 0;

    loadBitfield(file);

    while (1) {
        if (getBit(file) == 1) {
            buffer[pos++] = static_cast<uint8_t>(readByte(file));

            // Don't write any more bytes if the buffer is full
            if (pos >= bufferSize) {
                return Result(false, pos);
            }

            continue;
        }

        if (getBit(file) == 1) {
            const uint8_t lo = static_cast<uint8_t>(readByte(file));
            const uint8_t hi = static_cast<uint8_t>(readByte(file));

            // ---hi--- ---lo---
            // OOOOOCCC OOOOOOOO [CCCCCCCC]
            // O - Offset bit
            // C - Count bit

            // Offsets are negative numbers stored using a 13-bit two's complement representation. Before the offset
            // Can be applied to a 16-bit offset, we have to convert it to 16-bit two's complement representation.
            // Positive numbers are not used, so a naive conversion is okay.
            offset = (0xFF00 | hi) << 5;
            offset = (offset & 0xFF00) | lo;

            // Mask off the count bits
            count = hi & 0x7;

            if (count == 0) {
                count = static_cast<uint16_t>(readByte(file));

                if (count == 0) {
                    break;
                }

                if (count <= 1) {
                    continue;
                }
            } else {
                count++;
            }
        } else {
            count = (getBit(file) << 1) | getBit(file);
            count++;

            // Convert 8-bit two's complement representation to 16-bit representation
            offset = static_cast<int16_t>(readByte(file)) | 0xFF00;
        }

        count++;

        // Copy 'count' bytes from the specified 'offset', relative to the current buffer position
        while (count > 0) {
            buffer[pos] = buffer[pos + offset];
            pos++;
            count--;

            // Don't write any more bytes if the buffer is full
            if (pos > bufferSize) {
                return Result(false, pos);
            }
        }
    }

    return Result(true, pos);
}
