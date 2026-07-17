#pragma once

#include <cstddef>
#include <cstdint>

/**
 * Sonic 2/3 block descriptor
 *
 * A Sonic block descriptor is used to specify which block to draw and how it should be drawn. A block may be
 * horizontally and/or vertically flipped.
 *
 * A pattern descriptor is defined as a 16-bit bitmask, in the form:
 *
 *  ???? YXII IIII IIII
 *
 *  Masks:
 *   0x3FF block index
 *   0x400 X flip
 *   0x800 Y flip
 */
class BlockDesc
{
public:
    BlockDesc();

    uint16_t get() const;
    uint16_t getBlockIndex() const;

    bool getHFlip() const;
    bool getVFlip() const;

    void set(uint16_t);
    void set(BlockDesc& desc);

    static size_t getIndexSize();

private:
    BlockDesc(const BlockDesc&) = delete;
    BlockDesc& operator=(const BlockDesc&) = delete;

    uint16_t index_;
};

inline BlockDesc::BlockDesc()
    : index_(0)
{

}

inline uint16_t BlockDesc::get() const
{
    return index_;
}

inline uint16_t BlockDesc::getBlockIndex() const
{
    return index_ & 0x3FF;
}

inline bool BlockDesc::getHFlip() const
{
    return (index_ & 0x400) != 0;
}

inline bool BlockDesc::getVFlip() const
{
    return (index_ & 0x800) != 0;
}

inline void BlockDesc::set(uint16_t value)
{
    index_ = value;
}

inline void BlockDesc::set(BlockDesc& desc)
{
    index_ = desc.index_;
}

inline size_t BlockDesc::getIndexSize()
{
    return sizeof(uint16_t);
}
