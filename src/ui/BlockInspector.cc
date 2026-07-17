#include <cmath>
#include <iostream>

#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>

#include "../Block.h"
#include "../Level.h"
#include "../Logger.h"
#include "../Palette.h"
#include "../Pattern.h"

#include "BlockInspector.h"

#undef LOG
#define LOG Logger("BlockInspector")

using namespace std;

static constexpr int PIXMAP_WIDTH = 320;
static constexpr int BLOCKS_PER_ROW = PIXMAP_WIDTH / Block::BLOCK_WIDTH;

BlockInspector::BlockInspector(QWidget* parent, const shared_ptr<Level>& level)
    : QDialog(parent)
    , level_(level)
    , pixmap_(nullptr)
{
    const auto blockCount = level->getBlockCount();
    const int pixmapHeight = ceilf(static_cast<float>(blockCount) / BLOCKS_PER_ROW) * Block::BLOCK_HEIGHT;

    // main layout
    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->setContentsMargins(8, 8, 8, 8);
    vbox->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(vbox);

    // create widget to display pixmap
    label_ = new QLabel();
    label_->setFixedSize(PIXMAP_WIDTH, pixmapHeight);
    label_->setMinimumWidth(PIXMAP_WIDTH);
    vbox->addWidget(label_);

    // create pixmap
    pixmap_ = new QPixmap(PIXMAP_WIDTH, pixmapHeight);
    label_->setPixmap(*pixmap_);
    drawBlocks();
}

void BlockInspector::drawPattern(QImage& image,
                                 const Pattern& pattern,
                                 const Palette& palette,
                                 int dx,
                                 int dy,
                                 bool hFlip,
                                 bool vFlip)
{
    for (int py = 0; py < Pattern::PATTERN_HEIGHT; py++) {
        for (int px = 0; px < Pattern::PATTERN_WIDTH; px++) {
            const auto fx = hFlip ? 7 - px : px;
            const auto fy = vFlip ? 7 - py : py;

            const auto idx = pattern.getPixel(fx, fy);
            const auto color = palette.getColor(idx);

            image.setPixel(dx + px, dy + py, qRgb(color.r, color.g, color.b));
        }
    }
}

void BlockInspector::drawBlock(QImage& image, const Block& block, int dx, int dy)
{
    for (int py = 0; py < 2; py++) {
        for (int px = 0; px < 2; px++) {
            const auto& patternDesc = block.getPatternDesc(px, py);

            const auto paletteIndex = patternDesc.getPaletteIndex();
            const auto patternIndex = patternDesc.getPatternIndex();

            const auto& pattern = level_->getPattern(patternIndex);
            const auto& palette = level_->getPalette(paletteIndex);

            drawPattern(image,
                  pattern,
                  palette,
                  dx + px * Pattern::PATTERN_WIDTH,
                  dy + py * Pattern::PATTERN_HEIGHT,
                  patternDesc.getHFlip(),
                  patternDesc.getVFlip());
        }
    }
}

void BlockInspector::drawBlocks()
{
    LOG() << "Drawing blocks";

    // image to draw to
    QImage image(pixmap_->width(), pixmap_->height(), QImage::Format_RGB888);
    image.fill(qRgb(0, 0, 0));

    // draw individual blocks
    for (size_t i = 0; i < level_->getBlockCount(); i++) {
        const auto row = static_cast<int>(i / BLOCKS_PER_ROW);
        const auto col = static_cast<int>(i % BLOCKS_PER_ROW);

        drawBlock(image, level_->getBlock(i), col * Block::BLOCK_WIDTH, row * Block::BLOCK_HEIGHT);
    }

    // copy to pixmap
    LOG() << "Copying pattern image to pixmap";
    if (pixmap_->convertFromImage(image)) {
        label_->setPixmap(*pixmap_);
    } else {
        LOG() << "Failed to copy image to pixmap";
    }
}

void BlockInspector::refresh()
{
    drawBlocks();
}
