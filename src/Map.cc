#include <cstring>
#include <stdexcept>

#include "Map.h"

using namespace std;

Map::Map(uint8_t layers, uint16_t width, uint16_t height)
    : Map(layers, width, height, nullptr)
{

}

Map::Map(uint8_t layers, uint16_t width, uint16_t height, uint8_t* data)
    : layers_(layers)
    , height_(height)
    , width_(width)
{
    const size_t size = sizeof(uint8_t) * layers * width * height;

    data_ = new uint8_t[size];
    if (!data_) {
        throw runtime_error("Failed to allocate memory for level map");
    }

    if (data) {
        memcpy(data_, data, size);
    } else {
        memset(data_, 0, size);
    }

    layers_ = layers;
    width_ = width;
    height_ = height;
}

Map::~Map()
{
    if (data_) {
        delete[] data_;
        data_ = nullptr;
    }
}

uint8_t Map::getValue(uint8_t layer, uint16_t x, uint16_t y) const
{
    if (layer >= layers_) {
        throw runtime_error("Invalid map layer index");
    }

    if (x >= width_ || y >= height_) {
        throw runtime_error("Invalid map tile index");
    }

    return data_[y * width_ * layers_ + layer * width_ + x];
}

void Map::setValue(uint8_t layer, uint16_t x, uint16_t y, uint8_t value)
{
    if (layer >= layers_) {
        throw runtime_error("Invalid map layer index");
    }

    if (x >= width_ || y >= height_) {
        throw runtime_error("Invalid map tile index");
    }

    data_[y * width_ * layers_ + layer * width_ + x] = value;
}

uint8_t* Map::getData()
{
    return data_;
}
