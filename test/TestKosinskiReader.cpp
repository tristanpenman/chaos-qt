#include <gtest/gtest.h>

#include <QBuffer>

#include "../src/KosinskiReader.h"

#include "data/ehz1.kosinski.h"
#include "data/ehz1.raw.h"

class TestKosinskiReader : public testing::Test
{

};

TEST_F(TestKosinskiReader, ThrowOnEmptyStream)
{
  uint8_t buffer[16];

  QBuffer bufferDevice;
  bufferDevice.open(QIODevice::ReadOnly);
  KosinskiReader reader;
  EXPECT_ANY_THROW(reader.decompress(bufferDevice, buffer, 16));
}

TEST_F(TestKosinskiReader, ThrowOnIncompleteSteam)
{
  uint8_t buffer[16];

  // case 1: literal bytes
  {
    QByteArray data;
    data.append(char(0xff));
    data.append(char(0xff));
    QBuffer bufferDevice(&data);
    bufferDevice.open(QIODevice::ReadOnly);
    KosinskiReader reader;
    EXPECT_ANY_THROW(reader.decompress(bufferDevice, buffer, 16));
  }

  // case 2: various
  {
    QByteArray data;
    data.append(char(0x33));
    data.append(char(0x66));
    QBuffer bufferDevice(&data);
    bufferDevice.open(QIODevice::ReadOnly);
    KosinskiReader reader;
    EXPECT_ANY_THROW(reader.decompress(bufferDevice, buffer, 16));
  }
}

TEST_F(TestKosinskiReader, ReturnFalseOnNullBuffer)
{
  QByteArray data;
  data.append(char(0xff));
  data.append(char(0xff));
  data.append(char(0x01));
  data.append(char(0x02));
  QBuffer bufferDevice(&data);
  bufferDevice.open(QIODevice::ReadOnly);
  KosinskiReader reader;
  auto result = reader.decompress(bufferDevice, nullptr, 0);
  EXPECT_FALSE(result.first);
  EXPECT_EQ(0, result.second);
}

TEST_F(TestKosinskiReader, ReturnFalseOnBufferOverflow)
{
  uint8_t buffer[1];

  // case 1: literal byte mode
  {
    QByteArray data;
    data.append(char(0xff));
    data.append(char(0xff));
    data.append(char(0x01));
    data.append(char(0x02));
    QBuffer bufferDevice(&data);
    bufferDevice.open(QIODevice::ReadOnly);
    KosinskiReader reader;
    auto result = reader.decompress(bufferDevice, buffer, 1);
    EXPECT_FALSE(result.first);
    EXPECT_EQ(1, result.second);
  }
}

TEST_F(TestKosinskiReader, ReturnTrueOnHappyPath)
{
  std::vector<uint8_t> buffer(sizeof(ehz1_raw));
  QByteArray data(reinterpret_cast<const char*>(ehz1_kosinski), sizeof(ehz1_kosinski));
  QBuffer bufferDevice(&data);
  bufferDevice.open(QIODevice::ReadOnly);
  KosinskiReader reader;
  auto result = reader.decompress(bufferDevice, buffer.data(), buffer.size());
  EXPECT_TRUE(result.first);
  EXPECT_EQ(sizeof(ehz1_raw), result.second);
}
