#include <gtest/gtest.h>

#include <QBuffer>

#include "../src/KosinskiReader.h"
#include "../src/KosinskiWriter.h"

#include "data/ehz1.kosinski.h"
#include "data/ehz1.raw.h"

class TestKosinskiWriter : public testing::Test
{

};

TEST_F(TestKosinskiWriter, ThrowOnNullData)
{
    QBuffer os;
    os.open(QIODevice::ReadWrite);
    KosinskiWriter writer;
    KosinskiWriter::Result result;
    EXPECT_ANY_THROW(writer.compress(os, nullptr, sizeof(ehz1_raw)));
}

TEST_F(TestKosinskiWriter, ReturnFalseWhenByteLimitExceeded)
{
    // 1: too small to reserve bitfield
    {
        QBuffer os;
        os.open(QIODevice::ReadWrite);
        KosinskiWriter writer;
        KosinskiWriter::Result result;
        EXPECT_NO_THROW(result = writer.compress(os, reinterpret_cast<const uint8_t*>(ehz1_raw), sizeof(ehz1_raw), 1));
        EXPECT_FALSE(result.first);
        EXPECT_EQ(0, os.pos());
        EXPECT_TRUE(os.isOpen());
    }

    // 2: too small for compressed data
    {
        QBuffer os;
    os.open(QIODevice::ReadWrite);
        KosinskiWriter writer;
        KosinskiWriter::Result result;
        EXPECT_NO_THROW(result = writer.compress(os, reinterpret_cast<const uint8_t*>(ehz1_raw), sizeof(ehz1_raw), 5));
        EXPECT_FALSE(result.first);
        EXPECT_EQ(0, os.pos());
        EXPECT_TRUE(os.isOpen());
    }
}

TEST_F(TestKosinskiWriter, ReturnTrueOnHappyPath)
{
    QBuffer os;
    os.open(QIODevice::ReadWrite);

    // 1: compress
    {
        KosinskiWriter writer;
        KosinskiWriter::Result result;
        EXPECT_NO_THROW(result = writer.compress(os, reinterpret_cast<const uint8_t*>(ehz1_raw), sizeof(ehz1_raw)));
        EXPECT_TRUE(result.first);
        EXPECT_TRUE(os.isOpen());
    }

    // 2: decompress
    {
        std::vector<uint8_t> buffer(sizeof(ehz1_raw));
        KosinskiReader reader;
        KosinskiReader::Result result;
        os.seek(0);
        EXPECT_NO_THROW(result = reader.decompress(os, buffer.data(), buffer.size()));
        EXPECT_TRUE(result.first);
        EXPECT_EQ(sizeof(ehz1_raw), result.second);
        for (int i = 0; i < sizeof(ehz1_raw); i++) {
            EXPECT_EQ(ehz1_raw[i], buffer[i]);
        }
    }
}

TEST_F(TestKosinskiWriter, ReturnTrueOnSingleByteInput)
{
    const uint8_t data[] = { 0x5a };
    QBuffer os;
    os.open(QIODevice::ReadWrite);

    KosinskiWriter writer;
    KosinskiWriter::Result result;
    EXPECT_NO_THROW(result = writer.compress(os, data, sizeof(data)));

    EXPECT_TRUE(result.first);
    EXPECT_TRUE(os.isOpen());

    const QByteArray expected = QByteArray()
        .append(char(0x05))
        .append(char(0x00))
        .append(char(0x5a))
        .append(char(0xff))
        .append(char(0xf8))
        .append(char(0x00));
    EXPECT_EQ(expected, os.data());
}

TEST_F(TestKosinskiWriter, ReturnTrueWhenFinalByteIsLiteral)
{
    const uint8_t data[] = { 0x5a, 0xa5 };
    QBuffer os;
    os.open(QIODevice::ReadWrite);

    KosinskiWriter writer;
    KosinskiWriter::Result result;
    EXPECT_NO_THROW(result = writer.compress(os, data, sizeof(data)));
    EXPECT_TRUE(result.first);

    std::vector<uint8_t> buffer(sizeof(data));
    KosinskiReader reader;
    os.seek(0);
    EXPECT_NO_THROW(result = reader.decompress(os, buffer.data(), buffer.size()));
    EXPECT_TRUE(result.first);
    EXPECT_EQ(sizeof(data), result.second);
    EXPECT_EQ(data[0], buffer[0]);
    EXPECT_EQ(data[1], buffer[1]);
}

TEST_F(TestKosinskiWriter, ReturnTrueOnHappyPathWithByteLimit)
{
    QBuffer os;
    os.open(QIODevice::ReadWrite);
    KosinskiWriter writer;
    KosinskiWriter::Result result;
    EXPECT_NO_THROW(result = writer.compress(os, reinterpret_cast<const uint8_t*>(ehz1_raw), sizeof(ehz1_raw), 512));
    EXPECT_TRUE(result.first);
    EXPECT_TRUE(os.isOpen());
}
