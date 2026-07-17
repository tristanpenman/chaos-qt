#include <gtest/gtest.h>

#include "../src/games/Sonic2RingLayout.h"

TEST(Sonic2RingLayout, ReadsHorizontalAndVerticalGroups)
{
  const std::vector<uint8_t> data = {
      0x01, 0x20, 0x20, 0x80,
      0x02, 0x00, 0xb1, 0x40,
      0xff, 0xff
  };

  const auto groups = Sonic2RingLayout::read(data);
  ASSERT_EQ(2, groups.size());

  EXPECT_EQ(0x120, groups[0].x);
  EXPECT_EQ(0x080, groups[0].y);
  EXPECT_EQ(3, groups[0].count);
  EXPECT_EQ(RingDirection::kHorizontal, groups[0].direction);

  EXPECT_EQ(0x200, groups[1].x);
  EXPECT_EQ(0x140, groups[1].y);
  EXPECT_EQ(4, groups[1].count);
  EXPECT_EQ(RingDirection::kVertical, groups[1].direction);
}

TEST(Sonic2RingLayout, StopsAtTwoByteTerminator)
{
  const std::vector<uint8_t> data = {0xff, 0xff, 0x12, 0x34};
  EXPECT_TRUE(Sonic2RingLayout::read(data).empty());
}

TEST(Sonic2RingLayout, RejectsTruncatedEntry)
{
  const std::vector<uint8_t> data = {0x01, 0x20, 0x30};
  EXPECT_THROW(Sonic2RingLayout::read(data), std::runtime_error);
}

TEST(Sonic2RingLayout, RejectsMissingTerminator)
{
  const std::vector<uint8_t> data = {0x01, 0x20, 0x00, 0x80};
  EXPECT_THROW(Sonic2RingLayout::read(data), std::runtime_error);
}

TEST(Sonic2RingLayout, PreservesGroupsWithDecreasingXPositions)
{
  const std::vector<uint8_t> data = {
      0x02, 0x00, 0x00, 0x80,
      0x01, 0x20, 0x00, 0x80,
      0xff, 0xff
  };

  const auto groups = Sonic2RingLayout::read(data);
  ASSERT_EQ(2, groups.size());
  EXPECT_EQ(0x200, groups[0].x);
  EXPECT_EQ(0x120, groups[1].x);
}
