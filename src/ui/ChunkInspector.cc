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
    , m_level(level)
    , m_chunkIndex(0)
{
    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->setContentsMargins(8, 8, 8, 8);
    setLayout(vbox);

    // chunk selector
    QComboBox* chunkCombo = new QComboBox();
    vbox->addWidget(chunkCombo);
    for (size_t i = 0; i < m_level->getChunkCount(); i++) {
        const QString paletteName = tr("Chunk %1").arg(i);
        chunkCombo->addItem(paletteName, QVariant::fromValue(i));
    }

    // create widget to display pixmap
    m_label = new QLabel();
    m_label->setFixedSize(Chunk::CHUNK_WIDTH, Chunk::CHUNK_HEIGHT);
    m_label->setMinimumWidth(Chunk::CHUNK_WIDTH);
    vbox->addWidget(m_label);

    // create pixmap
    m_pixmap = new QPixmap(Chunk::CHUNK_WIDTH, Chunk::CHUNK_HEIGHT);
    m_label->setPixmap(*m_pixmap);
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

void ChunkInspector::drawBlock(QImage& image, const Block& block, int dx, int dy, bool hFlip, bool vFlip)
{
    for (int py = 0; py < 2; py++) {
        for (int px = 0; px < 2; px++) {
            const auto& patternDesc = block.getPatternDesc(hFlip ? 1 - px : px, vFlip ? 1 - py : py);

            const auto paletteIndex = patternDesc.getPaletteIndex();
            const auto patternIndex = patternDesc.getPatternIndex();

            const auto& pattern = m_level->getPattern(patternIndex);
            const auto& palette = m_level->getPalette(paletteIndex);

            drawPattern(image,
                  pattern,
                  palette,
                  dx + px * Pattern::PATTERN_WIDTH,
                  dy + py * Pattern::PATTERN_HEIGHT,
                  patternDesc.getHFlip() ^ hFlip,
                  patternDesc.getVFlip() ^ vFlip);
        }
    }
}

void ChunkInspector::drawChunk(size_t index)
{
    LOG() << "Drawing chunk " << index;

    const Chunk& chunk = m_level->getChunk(index);

    QImage image(Chunk::CHUNK_WIDTH, Chunk::CHUNK_HEIGHT, QImage::Format_RGB888);
    image.fill(0);

    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            const auto& blockDesc = chunk.getBlockDesc(dx, dy);
            const auto blockIndex = blockDesc.getBlockIndex();
            try {
                const auto& block = m_level->getBlock(blockIndex);
                drawBlock(image, block, dx * 16, dy * 16, blockDesc.getHFlip(), blockDesc.getVFlip());
            } catch (const exception& e) {
                LOG() << "Failed to draw block " << blockIndex << ": " << e.what();
            }
        }
    }

    // copy to pixmap
    LOG() << "Copying chunk image to pixmap";
    if (m_pixmap->convertFromImage(image)) {
        m_label->setPixmap(*m_pixmap);
    } else {
        LOG() << "Failed to copy image to pixmap";
    }
}

void ChunkInspector::chunkChanged(int index)
{
    m_chunkIndex = static_cast<size_t>(index);
    drawChunk(index);
}

void ChunkInspector::refresh()
{
    drawChunk(m_chunkIndex);
}
