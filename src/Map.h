#pragma once

#include <cstdint>

class Map
{
public:
    Map(uint8_t layers, uint16_t width, uint16_t height);
    Map(uint8_t layers, uint16_t width, uint16_t height, uint8_t* data);

    ~Map();

    uint16_t getWidth() const;
    uint16_t getHeight() const;
    uint8_t getLayerCount() const;

    uint8_t getValue(uint8_t layer, uint16_t x, uint16_t y) const;
    void setValue(uint8_t layer, uint16_t x, uint16_t y, uint8_t);

    uint8_t* getData();

protected:
    uint8_t layers_;
    uint16_t height_;
    uint16_t width_;

    uint8_t* data_;
};

inline uint16_t Map::getWidth() const
{
    return width_;
}

inline uint16_t Map::getHeight() const
{
    return height_;
}

inline uint8_t Map::getLayerCount() const
{
    return layers_;
}
