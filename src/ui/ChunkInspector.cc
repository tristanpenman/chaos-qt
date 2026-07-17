#include "ChunkInspector.h"

#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

#include "../Chunk.h"
#include "../Block.h"
#include "../Level.h"
#include "../Logger.h"
#include "../Palette.h"
#include "../Pattern.h"

#undef LOG
#define LOG Logger("ChunkInspector")

using namespace std;

ChunkInspector::ChunkInspector(QWidget* parent, const shared_ptr<Level>& level)
    : QDialog(parent)
    , level_(level)
    , chunkIndex_(0)
{
    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->setContentsMargins(8, 8, 8, 8);
    setLayout(vbox);

    // chunk selector
    QComboBox* chunkCombo = new QComboBox();
    vbox->addWidget(chunkCombo);
    for (size_t i = 0; i < level_->getChunkCount(); i++) {
        const QString paletteName = tr("Chunk %1").arg(i);
        chunkCombo->addItem(paletteName, QVariant::fromValue(i));
    }

    // create widget to display pixmap
    label_ = new QLabel();
    label_->setFixedSize(Chunk::kChunkWidth, Chunk::kChunkHeight);
    label_->setMinimumWidth(Chunk::kChunkWidth);
    vbox->addWidget(label_);

    // create pixmap
    pixmap_ = new QPixmap(Chunk::kChunkWidth, Chunk::kChunkHeight);
    label_->setPixmap(*pixmap_);
    drawChunk(0);

    // handle switching chunks
    connect(chunkCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ChunkInspector::chunkChanged);
}

void ChunkInspector::drawPattern(QImage& image,
                                 const Pattern& pattern,
                                 const Palette& palette,
                                 int dx,
                                 int dy,
                                 bool hFlip,
                                 bool vFlip)
{
    for (int py = 0; py < Pattern::kPatternHeight; py++) {
        for (int px = 0; px < Pattern::kPatternWidth; px++) {
            const auto fx = hFlip ? 7 - px : px;
            const auto fy = vFlip ? 7 - py : py;

            const auto idx = pattern.getPixel(fx, fy);
            const auto color = palette.getColor(idx);

            image.setPixel(dx + px, dy + py, qRgb(color.r, color.g, color.b));
        }
    }
}

void ChunkInspector::drawBlock(QImage& image, const Block& block, int dx, int dy, bool hFlip, bool vFlip)
{
    for (int py = 0; py < 2; py++) {
        for (int px = 0; px < 2; px++) {
            const auto& patternDesc = block.getPatternDesc(hFlip ? 1 - px : px, vFlip ? 1 - py : py);

            const auto paletteIndex = patternDesc.getPaletteIndex();
            const auto patternIndex = patternDesc.getPatternIndex();

            const auto& pattern = level_->getPattern(patternIndex);
            const auto& palette = level_->getPalette(paletteIndex);

            drawPattern(image,
                  pattern,
                  palette,
                  dx + px * Pattern::kPatternWidth,
                  dy + py * Pattern::kPatternHeight,
                  patternDesc.getHFlip() ^ hFlip,
                  patternDesc.getVFlip() ^ vFlip);
        }
    }
}

void ChunkInspector::drawChunk(size_t index)
{
    LOG() << "Drawing chunk " << index;

    const Chunk& chunk = level_->getChunk(index);

    QImage image(Chunk::kChunkWidth, Chunk::kChunkHeight, QImage::Format_RGB888);
    image.fill(0);

    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            const auto& blockDesc = chunk.getBlockDesc(dx, dy);
            const auto blockIndex = blockDesc.getBlockIndex();
            try {
                const auto& block = level_->getBlock(blockIndex);
                drawBlock(image, block, dx * 16, dy * 16, blockDesc.getHFlip(), blockDesc.getVFlip());
            } catch (const exception& e) {
                LOG() << "Failed to draw block " << blockIndex << ": " << e.what();
            }
        }
    }

    // copy to pixmap
    LOG() << "Copying chunk image to pixmap";
    if (pixmap_->convertFromImage(image)) {
        label_->setPixmap(*pixmap_);
    } else {
        LOG() << "Failed to copy image to pixmap";
    }
}

void ChunkInspector::chunkChanged(int index)
{
    chunkIndex_ = static_cast<size_t>(index);
    drawChunk(index);
}

void ChunkInspector::refresh()
{
    drawChunk(chunkIndex_);
}
