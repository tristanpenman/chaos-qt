#include "KosinskiWriter.h"



void KosinskiWriter::writeByte(uint8_t byte)
{
    buffer_.push_back(byte);
}

void KosinskiWriter::addBit(int bit)
{
    bitfield_ |= bit << bitcount_;
    bitcount_++;

    if (bitcount_ == 16) {
        // Fill in bitfield
        buffer_[bitfieldPos_] = bitfield_ & 0xff;
        buffer_[bitfieldPos_ + 1] = (bitfield_ >> 8) & 0xff;

        // Prepare next bitfield
        bitfieldPos_ = buffer_.size();
        buffer_.push_back(0);
        buffer_.push_back(0);

        // Reset bitfield
        bitfield_ = 0;
        bitcount_ = 0;
    }
}

bool KosinskiWriter::segmentsEqual(const uint8_t data[], int32_t start, int32_t length, int32_t offset)
{
    for (int i = 0; i < length; i++) {
        if (data[start + i] != data[start + i + offset]) {
            return false;
        }
    }

    return true;
}

int32_t KosinskiWriter::findSegment(const uint8_t data[], int32_t pos, int32_t length)
{
    int32_t curPos = pos - length;

    for (;;) {
        if (curPos < 0) {
            return 0;
        }

        if (segmentsEqual(data, pos, length, curPos - pos)) {
            return curPos - pos;
        } else {
            curPos--;
        }

        if (pos - curPos > 4096) {
            return 0;
        }
    }
}

KosinskiWriter::Result KosinskiWriter::compress(QIODevice& file,
                                                const uint8_t data[],
                                                size_t dataSize,
                                                std::optional<size_t> byteLimit)
{
    if (!data) {
        throw std::runtime_error("Invalid data pointer");
    }

    buffer_.clear();

    bitcount_ = 0;
    bitfield_ = 0;
    bitfields_ = 0;
    bitfieldPos_ = 0;

    int32_t pos = 0;
    int32_t length = 0;
    int32_t offset = 0;

    // Space for initial bitfield
    buffer_.push_back(0);
    buffer_.push_back(0);

    while (pos < int32_t(dataSize)) {
        length = 1;
        offset = 0;

        // 1. capture any repeating bytes

        while (pos + length < int32_t(dataSize)) {
            // Maximum run length is 256 bytes
            if (length >= 256) {
                break;
            }

            // Break once bytes stop repeating
            if (data[pos] != data[pos + length]) {
                break;
            }

            length++;
        }

        // 2. can we copy this sequence from somewhere?

        int32_t tmpOffset = findSegment(data, pos, length);
        while (tmpOffset != 0) {
            offset = tmpOffset;
            length++;

            if (pos + length > int32_t(dataSize)) {
                break;
            }

            if (length > 256) {
                break;
            }

            tmpOffset = findSegment(data, pos, length);
        }

        // 3. if there is a copy location, check if it is adjacent to this one
        if (offset != 0) {
            length--;

            if (length + offset == 0) {
                int32_t segLength = length;

                while (pos + length < int32_t(dataSize)) {
                    if (data[pos + length - segLength] != data[pos + length]) {
                        break;
                    }

                    if (pos + length + 1 > int32_t(dataSize)) {
                        break;
                    }

                    if (length + 1 > 256) {
                        break;
                    }

                    length++;
                }
            }
        } else if (pos > 0 && data[pos - 1] == data[pos]) {
            // No copy loc... is the previous byte the same as
            // This? If so, use it to do the replication.
            offset = -1;
        }

        if (offset == 0 || ((length <= 2) && (offset < -127)) || length < 2) {
            // Spit out a byte if there was no sizeable run
            addBit(1);

            if (length == 2) {
                length = 1;
            }

            writeByte(data[pos++]);
            offset = -1;

            if ((data[pos] != data[pos + length - 1]) || length == 1) {
                length--;
            }
        }

        if (length == 0) {
            continue;
        }

        // Seems we have a run. Time to encode it

        addBit(0);

        if ((length <= 5) && (offset >= -127)) {
            addBit(0);
            addBit(((length - 2) >> 1) & 1);
            addBit((length - 2) & 1);
            writeByte(static_cast<uint8_t>(offset));

        } else {
            addBit(1);

            int hi = 0;
            int lo = 0;
            if (length <= 9) {
                hi = (offset >> 5) & 0xF8;
                hi |= (length - 2) & 0x07;
            } else {
                hi = (offset >> 5) & 0xF8;
            }

            lo = offset & 0xFF;

            writeByte(static_cast<uint8_t>(lo));
            writeByte(static_cast<uint8_t>(hi));

            if (length > 9) {
                writeByte(static_cast<uint8_t>(length - 1));
            }
        }

        // Now, adjust our position and continue
        pos += length;
    }

    addBit(0);
    addBit(1);

    int hi = (offset >> 5) & 0xF8;
    int lo = offset & 0x00FF;
    writeByte(static_cast<uint8_t>(lo));
    writeByte(static_cast<uint8_t>(hi));
    writeByte(static_cast<uint8_t>(0));

    // Write final bitfield
    buffer_[bitfieldPos_] = bitfield_ & 0xff;
    buffer_[bitfieldPos_ + 1] = (bitfield_ >> 8) & 0xff;

    if (byteLimit && buffer_.size() > *byteLimit) {
        return Result(false, buffer_.size());
    }

    file.write(reinterpret_cast<const char*>(buffer_.data()), static_cast<qint64>(buffer_.size()));

    return Result(true, buffer_.size());
}
