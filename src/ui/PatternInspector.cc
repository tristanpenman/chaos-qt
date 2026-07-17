#include "PatternInspector.h"

#include <cmath>

#include <QComboBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>

#include "../Level.h"
#include "../Logger.h"
#include "../Palette.h"
#include "../Pattern.h"

#undef LOG
#define LOG Logger("PatternInspector")

using namespace std;

static constexpr int PIXMAP_WIDTH = 320;
static constexpr int PATTERNS_PER_ROW = PIXMAP_WIDTH / Pattern::PATTERN_WIDTH;

PatternInspector::PatternInspector(QWidget* parent, const shared_ptr<Level>& level)
    : QDialog(parent)
    , level_(level)
    , pixmap_(nullptr)
    , paletteIndex_(0)
{
    const auto patternCount = level->getPatternCount();
    const int pixmapHeight = ceilf(static_cast<float>(patternCount) / PATTERNS_PER_ROW) * Pattern::PATTERN_HEIGHT;

    // main layout
    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->setContentsMargins(8, 8, 8, 8);
    vbox->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(vbox);

    // palette combo box
    QComboBox* paletteCombo = new QComboBox();
    vbox->addWidget(paletteCombo);
    for (size_t i = 0; i < level->getPaletteCount(); i++) {
        const QString paletteName = tr("Palette %1").arg(i);
        paletteCombo->addItem(paletteName, QVariant::fromValue(i));
    }

    // create widget to display pixmap
    label_ = new QLabel();
    label_->setFixedSize(PIXMAP_WIDTH, pixmapHeight);
    label_->setMinimumWidth(PIXMAP_WIDTH);
    vbox->addWidget(label_);

    // create pixmap
    pixmap_ = new QPixmap(PIXMAP_WIDTH, pixmapHeight);
    label_->setPixmap(*pixmap_);
    drawPatterns(0);

    // handle switching palettes
    connect(paletteCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &PatternInspector::paletteChanged);
}

void PatternInspector::drawPattern(QImage& image, const Pattern& pattern, const Palette& palette, int dx, int dy)
{
    for (int py = 0; py < 8; py++) {
        for (int px = 0; px < 8; px++) {
            const auto idx = pattern.getPixel(px, py);
            const auto color = palette.getColor(idx);

            image.setPixel(dx + px, dy + py, qRgb(color.r, color.g, color.b));
        }
    }
}

void PatternInspector::drawPatterns(size_t paletteIndex)
{
    LOG() << "Drawing patterns using palette " << paletteIndex;

    const Palette& palette = level_->getPalette(paletteIndex);

    // image to draw to
    QImage image(pixmap_->width(), pixmap_->height(), QImage::Format_RGB888);
    image.fill(qRgb(0, 0, 0));

    // draw individual patterns
    for (size_t i = 0; i < level_->getPatternCount(); i++) {
        const auto row = static_cast<int>(i / PATTERNS_PER_ROW);
        const auto col = static_cast<int>(i % PATTERNS_PER_ROW);

        drawPattern(image, level_->getPattern(i), palette, col * Pattern::PATTERN_WIDTH, row * Pattern::PATTERN_HEIGHT);
    }

    // copy to pixmap
    LOG() << "Copying pattern image to pixmap";
    if (pixmap_->convertFromImage(image)) {
        label_->setPixmap(*pixmap_);
    } else {
        LOG() << "Failed to copy image to pixmap";
    }
}

void PatternInspector::paletteChanged(int paletteIndex)
{
    paletteIndex_ = static_cast<size_t>(paletteIndex);
    drawPatterns(paletteIndex);
}

void PatternInspector::refresh()
{
    drawPatterns(paletteIndex_);
}
