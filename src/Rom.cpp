#include <stdexcept>
#include <string>

#include <QByteArray>
#include <QString>

#include "Rom.h"

#define CHECKSUM_OFFSET 0x018E
#define CHECKSUM_BUFFER_SIZE 0x8000  // 32kB
#define ROM_HEADER_OFFSET 0x100
#define ROM_LENGTH_OFFSET 0x01A4
#define DOMESTIC_NAME_LEN 48
#define DOMESTIC_NAME_OFFSET ROM_HEADER_OFFSET + 32
#define INTERNATIONAL_NAME_LEN 48
#define INTERNATIONAL_NAME_OFFSET DOMESTIC_NAME_OFFSET + DOMESTIC_NAME_LEN

using namespace std;

bool Rom::open(const string& path)
{
  m_file.setFileName(QString::fromStdString(path));
  return m_file.open(QIODevice::ReadWrite);
}

QFile& Rom::getFile()
{
  return m_file;
}

size_t Rom::getSize()
{
  return static_cast<size_t>(m_file.size());
}

uint32_t Rom::readAddrRange()
{
  return read32BitAddr(0x1A4);
}

void Rom::writeSize(uint32_t size)
{
  write32BitAddr(size, 0x1A4);

  m_file.flush();
}

uint16_t Rom::calculateChecksum()
{
  m_file.seek(512);
  int count = 0;
  while (!m_file.atEnd()) {
    const QByteArray buffer = m_file.read(CHECKSUM_BUFFER_SIZE);
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
  return read16BitAddr(CHECKSUM_OFFSET);
}

void Rom::writeChecksum(uint16_t checksum)
{
  write16BitAddr(checksum, CHECKSUM_OFFSET);

  m_file.flush();
}

string Rom::readDomesticName()
{
  m_file.seek(DOMESTIC_NAME_OFFSET);
  QByteArray buffer = m_file.read(DOMESTIC_NAME_LEN);
  buffer.append('\0');
  return buffer.constData();
}

string Rom::readInternationalName()
{
  m_file.seek(INTERNATIONAL_NAME_OFFSET);
  QByteArray buffer = m_file.read(INTERNATIONAL_NAME_LEN);
  buffer.append('\0');
  return buffer.constData();
}

uint8_t Rom::readByte(streamoff offset)
{
  m_file.seek(offset);
  char value = 0;
  m_file.getChar(&value);

  return static_cast<uint8_t>(value);
}

vector<char> Rom::readBytes(streamoff offset, size_t count)
{
  m_file.seek(offset);
  const QByteArray data = m_file.read(static_cast<qint64>(count));
  return vector<char>(data.begin(), data.end());
}

uint16_t Rom::read16BitAddr(streamoff offset)
{
  m_file.seek(offset);

  char byte = 0;
  m_file.getChar(&byte);
  uint16_t addr = static_cast<uint8_t>(byte) << 8;
  m_file.getChar(&byte);
  addr |= static_cast<uint8_t>(byte);

  return addr;
}

uint32_t Rom::read32BitAddr(streamoff offset)
{
  m_file.seek(offset);

  char byte = 0;
  m_file.getChar(&byte);
  uint32_t addr = static_cast<uint32_t>(static_cast<uint8_t>(byte)) << 24;
  m_file.getChar(&byte);
  addr |= static_cast<uint32_t>(static_cast<uint8_t>(byte)) << 16;
  m_file.getChar(&byte);
  addr |= static_cast<uint32_t>(static_cast<uint8_t>(byte)) << 8;
  m_file.getChar(&byte);
  addr |= static_cast<uint32_t>(static_cast<uint8_t>(byte));

  return addr;
}

void Rom::write16BitAddr(uint16_t addr, streamoff offset)
{
  m_file.seek(offset);

  const char bytes[] = {
    static_cast<char>((addr >> 8) & 0xFF),
    static_cast<char>((addr) & 0xFF)
  };
  m_file.write(bytes, sizeof(bytes));
}

void Rom::write32BitAddr(uint32_t addr, streamoff offset)
{
  m_file.seek(offset);

  const char bytes[] = {
    static_cast<char>((addr >> 24) & 0xFF),
    static_cast<char>((addr >> 16) & 0xFF),
    static_cast<char>((addr >> 8) & 0xFF),
    static_cast<char>((addr) & 0xFF)
  };
  m_file.write(bytes, sizeof(bytes));
}
