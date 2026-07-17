#include <stdexcept>
#include <string>

#include <QByteArray>
#include <QString>

#include "Rom.h"

using namespace std;

namespace {

constexpr uint32_t kChecksumOffset = 0x018E;
constexpr uint32_t kChecksumDataOffset = 0x0200;
constexpr qint64 kChecksumBufferSize = 0x8000;  // 32 KiB
constexpr uint32_t kRomHeaderOffset = 0x100;
constexpr uint32_t kRomLengthOffset = 0x01A4;
constexpr qint64 kDomesticNameLength = 48;
constexpr uint32_t kDomesticNameOffset = kRomHeaderOffset + 32;
constexpr qint64 kInternationalNameLength = 48;
constexpr uint32_t kInternationalNameOffset = kDomesticNameOffset + kDomesticNameLength;

}  // namespace

bool Rom::open(const string& path)
{
    file_.setFileName(QString::fromStdString(path));
    return file_.open(QIODevice::ReadWrite);
}

QFile& Rom::getFile()
{
    return file_;
}

size_t Rom::getSize()
{
    return static_cast<size_t>(file_.size());
}

uint32_t Rom::readAddrRange()
{
    return read32BitAddr(kRomLengthOffset);
}

void Rom::writeSize(uint32_t size)
{
    write32BitAddr(size, kRomLengthOffset);

    file_.flush();
}

uint16_t Rom::calculateChecksum()
{
    file_.seek(kChecksumDataOffset);
    int count = 0;
    while (!file_.atEnd()) {
        const QByteArray buffer = file_.read(kChecksumBufferSize);
        const auto readCount = buffer.size();
        for (auto i = 0; i < readCount; i += 2) {
            int num;

            if (buffer[i] < 0) {
                num = buffer[i] + 256;
            } else {
                num = buffer[i];
            }

            count += num << 8;

            if ((i + 1) < readCount) {
                if (buffer[i + 1] < 0) {
                    num = buffer[i + 1] + 256;
                } else {
                    num = buffer[i + 1];
                }

                count += num;
            }

            count &= 0xFFFF;
        }
    }

    return static_cast<uint16_t>(count);
}

uint16_t Rom::readChecksum()
{
    return read16BitAddr(kChecksumOffset);
}

void Rom::writeChecksum(uint16_t checksum)
{
    write16BitAddr(checksum, kChecksumOffset);

    file_.flush();
}

string Rom::readDomesticName()
{
    file_.seek(kDomesticNameOffset);
    QByteArray buffer = file_.read(kDomesticNameLength);
    buffer.append('\0');
    return buffer.constData();
}

string Rom::readInternationalName()
{
    file_.seek(kInternationalNameOffset);
    QByteArray buffer = file_.read(kInternationalNameLength);
    buffer.append('\0');
    return buffer.constData();
}

uint8_t Rom::readByte(streamoff offset)
{
    file_.seek(offset);
    char value = 0;
    file_.getChar(&value);

    return static_cast<uint8_t>(value);
}

vector<char> Rom::readBytes(streamoff offset, size_t count)
{
    file_.seek(offset);
    const QByteArray data = file_.read(static_cast<qint64>(count));
    return vector<char>(data.begin(), data.end());
}

uint16_t Rom::read16BitAddr(streamoff offset)
{
    file_.seek(offset);

    char byte = 0;
    file_.getChar(&byte);
    uint16_t addr = static_cast<uint8_t>(byte) << 8;
    file_.getChar(&byte);
    addr |= static_cast<uint8_t>(byte);

    return addr;
}

uint32_t Rom::read32BitAddr(streamoff offset)
{
    file_.seek(offset);

    char byte = 0;
    file_.getChar(&byte);
    uint32_t addr = static_cast<uint32_t>(static_cast<uint8_t>(byte)) << 24;
    file_.getChar(&byte);
    addr |= static_cast<uint32_t>(static_cast<uint8_t>(byte)) << 16;
    file_.getChar(&byte);
    addr |= static_cast<uint32_t>(static_cast<uint8_t>(byte)) << 8;
    file_.getChar(&byte);
    addr |= static_cast<uint32_t>(static_cast<uint8_t>(byte));

    return addr;
}

void Rom::write16BitAddr(uint16_t addr, streamoff offset)
{
    file_.seek(offset);

    const char bytes[] = {
        static_cast<char>((addr >> 8) & 0xFF),
        static_cast<char>((addr) & 0xFF)
    };
    file_.write(bytes, sizeof(bytes));
}

void Rom::write32BitAddr(uint32_t addr, streamoff offset)
{
    file_.seek(offset);

    const char bytes[] = {
        static_cast<char>((addr >> 24) & 0xFF),
        static_cast<char>((addr >> 16) & 0xFF),
        static_cast<char>((addr >> 8) & 0xFF),
        static_cast<char>((addr) & 0xFF)
    };
    file_.write(bytes, sizeof(bytes));
}
