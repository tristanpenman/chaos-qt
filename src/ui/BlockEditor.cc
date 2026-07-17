#include "BlockEditor.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "../Block.h"
#include "../Level.h"
#include "../Palette.h"
#include "../Pattern.h"

using namespace std;

static constexpr int CANVAS_SCALE = 10;
static constexpr int PATTERN_PREVIEW_SCALE = 2;
static constexpr int PATTERN_CELL_SIZE = Pattern::PATTERN_WIDTH * PATTERN_PREVIEW_SCALE;
static constexpr int PATTERN_ROW_HEIGHT = PATTERN_CELL_SIZE + 6;
static constexpr int PATTERN_LABEL_WIDTH = 84;
static constexpr uint16_t H_FLIP_MASK = 0x800;
static constexpr uint16_t V_FLIP_MASK = 0x1000;

static QColor toQColor(const Palette::Color& color)
{
    return QColor(color.r, color.g, color.b);
}

BlockCanvas::BlockCanvas(QWidget* parent, const shared_ptr<Level>& level, Block* blocks)
    : QWidget(parent)
    , level_(level)
    , blocks_(blocks)
    , blockIndex_(0)
    , selectedPatternIndex_(0)
    , selectedPaletteIndex_(0)
    , hFlip_(false)
    , vFlip_(false)
{
    setFixedSize(Block::BLOCK_WIDTH * CANVAS_SCALE, Block::BLOCK_HEIGHT * CANVAS_SCALE);
}

void BlockCanvas::setBlockIndex(size_t blockIndex)
{
    blockIndex_ = blockIndex;
    update();
}

void BlockCanvas::setSelectedPattern(uint16_t patternIndex, uint16_t paletteIndex, bool hFlip, bool vFlip)
{
    selectedPatternIndex_ = patternIndex;
    selectedPaletteIndex_ = paletteIndex;
    hFlip_ = hFlip;
    vFlip_ = vFlip;
}

void BlockCanvas::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    drawAt(event->position().toPoint());
#else
    drawAt(event->pos());
#endif
}

void BlockCanvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.scale(CANVAS_SCALE, CANVAS_SCALE);
    painter.fillRect(QRect(0, 0, Block::BLOCK_WIDTH, Block::BLOCK_HEIGHT), Qt::black);

    drawBlock(painter, blocks_[blockIndex_]);

    painter.resetTransform();
    painter.setPen(QColor(55, 55, 55));
    painter.drawRect(0, 0, width() - 1, height() - 1);
    painter.drawLine(Pattern::PATTERN_WIDTH * CANVAS_SCALE, 0, Pattern::PATTERN_WIDTH * CANVAS_SCALE, height());
    painter.drawLine(0, Pattern::PATTERN_HEIGHT * CANVAS_SCALE, width(), Pattern::PATTERN_HEIGHT * CANVAS_SCALE);
}

void BlockCanvas::drawAt(const QPoint& pos)
{
    const int x = pos.x() / (Pattern::PATTERN_WIDTH * CANVAS_SCALE);
    const int y = pos.y() / (Pattern::PATTERN_HEIGHT * CANVAS_SCALE);
    if (x < 0 || x > 1 || y < 0 || y > 1) {
        return;
    }

    Block& block = blocks_[blockIndex_];
    const uint16_t value = selectedPatternDescValue();
    if (block.getPatternDesc(static_cast<uint8_t>(x), static_cast<uint8_t>(y)).get() == value) {
        return;
    }

    block.setPatternDesc(static_cast<uint8_t>(x), static_cast<uint8_t>(y), value);
    update();
    emit blockModified();
}

void BlockCanvas::drawBlock(QPainter& painter, const Block& block)
{
    for (int py = 0; py < 2; py++) {
        for (int px = 0; px < 2; px++) {
            const auto& patternDesc = block.getPatternDesc(static_cast<uint8_t>(px), static_cast<uint8_t>(py));
            const auto& pattern = level_->getPattern(patternDesc.getPatternIndex());
            const auto& palette = level_->getPalette(patternDesc.getPaletteIndex());
            drawPattern(painter,
                  pattern,
                  palette,
                  px * Pattern::PATTERN_WIDTH,
                  py * Pattern::PATTERN_HEIGHT,
                  patternDesc.getHFlip(),
                  patternDesc.getVFlip());
        }
    }
}

void BlockCanvas::drawPattern(QPainter& painter,
                             const Pattern& pattern,
                             const Palette& palette,
                             int dx,
                             int dy,
                             bool hFlip,
                             bool vFlip)
{
    for (int py = 0; py < Pattern::PATTERN_HEIGHT; py++) {
        for (int px = 0; px < Pattern::PATTERN_WIDTH; px++) {
            const auto fx = hFlip ? Pattern::PATTERN_WIDTH - 1 - px : px;
            const auto fy = vFlip ? Pattern::PATTERN_HEIGHT - 1 - py : py;
            const auto color = palette.getColor(pattern.getPixel(static_cast<uint8_t>(fx), static_cast<uint8_t>(fy)));
            painter.fillRect(dx + px, dy + py, 1, 1, toQColor(color));
        }
    }
}

uint16_t BlockCanvas::selectedPatternDescValue() const
{
    uint16_t value = selectedPatternIndex_ & 0x7FF;
    value |= (selectedPaletteIndex_ & 0x3) << 13;
    if (hFlip_) {
        value |= H_FLIP_MASK;
    }
    if (vFlip_) {
        value |= V_FLIP_MASK;
    }
    return value;
}

PatternPaletteList::PatternPaletteList(QWidget* parent, const shared_ptr<Level>& level)
    : QWidget(parent)
    , level_(level)
    , selectedPatternIndex_(0)
    , selectedPaletteIndex_(0)
{
    setMinimumWidth(PATTERN_LABEL_WIDTH + PATTERN_CELL_SIZE * 4 + 16);
    setFixedHeight(static_cast<int>(level_->getPatternCount()) * PATTERN_ROW_HEIGHT);
    buildPixmapCache();
}

void PatternPaletteList::setSelected(uint16_t patternIndex, uint16_t paletteIndex)
{
    selectedPatternIndex_ = patternIndex;
    selectedPaletteIndex_ = paletteIndex;
    update();
}

void PatternPaletteList::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto pos = event->position().toPoint();
#else
    const auto pos = event->pos();
#endif
    const int patternIndex = pos.y() / PATTERN_ROW_HEIGHT;
    const int paletteIndex = (pos.x() - PATTERN_LABEL_WIDTH) / PATTERN_CELL_SIZE;
    if (patternIndex < 0 || patternIndex >= static_cast<int>(level_->getPatternCount()) ||
            paletteIndex < 0 || paletteIndex >= 4) {
        return;
    }

    selectedPatternIndex_ = static_cast<uint16_t>(patternIndex);
    selectedPaletteIndex_ = static_cast<uint16_t>(paletteIndex);
    update();
    emit patternSelected(selectedPatternIndex_, selectedPaletteIndex_);
}

void PatternPaletteList::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    painter.setRenderHint(QPainter::Antialiasing, false);

    for (size_t patternIndex = 0; patternIndex < level_->getPatternCount(); patternIndex++) {
        const int y = static_cast<int>(patternIndex) * PATTERN_ROW_HEIGHT;
        painter.setPen(palette().text().color());
        painter.drawText(QRect(0, y, PATTERN_LABEL_WIDTH - 8, PATTERN_ROW_HEIGHT), Qt::AlignVCenter | Qt::AlignRight, tr("Pattern %1").arg(patternIndex));

        for (size_t paletteIndex = 0; paletteIndex < 4; paletteIndex++) {
            const int x = PATTERN_LABEL_WIDTH + static_cast<int>(paletteIndex) * PATTERN_CELL_SIZE;
            painter.drawPixmap(x, y + 3, cachedPixmap(patternIndex, paletteIndex));

            if (patternIndex == selectedPatternIndex_ && paletteIndex == selectedPaletteIndex_) {
                painter.setPen(QPen(QColor(128, 192, 255), 2));
            } else {
                painter.setPen(QColor(55, 55, 55));
            }
            painter.drawRect(x, y + 3, PATTERN_CELL_SIZE - 1, PATTERN_CELL_SIZE - 1);
        }
    }
}

void PatternPaletteList::buildPixmapCache()
{
    pixmaps_.clear();
    pixmaps_.reserve(level_->getPatternCount() * 4);
    for (size_t patternIndex = 0; patternIndex < level_->getPatternCount(); patternIndex++) {
        for (size_t paletteIndex = 0; paletteIndex < 4; paletteIndex++) {
            pixmaps_.push_back(renderPatternPixmap(level_->getPattern(patternIndex), level_->getPalette(paletteIndex)));
        }
    }
}

QPixmap PatternPaletteList::renderPatternPixmap(const Pattern& pattern, const Palette& palette) const
{
    QImage image(Pattern::PATTERN_WIDTH, Pattern::PATTERN_HEIGHT, QImage::Format_RGB888);
    for (int py = 0; py < Pattern::PATTERN_HEIGHT; py++) {
        for (int px = 0; px < Pattern::PATTERN_WIDTH; px++) {
            const auto color = palette.getColor(pattern.getPixel(static_cast<uint8_t>(px), static_cast<uint8_t>(py)));
            image.setPixel(px, py, qRgb(color.r, color.g, color.b));
        }
    }

    return QPixmap::fromImage(image.scaled(PATTERN_CELL_SIZE,
                                         PATTERN_CELL_SIZE,
                                         Qt::IgnoreAspectRatio,
                                         Qt::FastTransformation));
}

const QPixmap& PatternPaletteList::cachedPixmap(size_t patternIndex, size_t paletteIndex) const
{
    return pixmaps_[patternIndex * 4 + paletteIndex];
}

BlockEditor::BlockEditor(QWidget* parent, const shared_ptr<Level>& level)
    : QDialog(parent)
    , level_(level)
    , blocks_(new Block[level->getBlockCount()])
    , blockCombo_(nullptr)
    , hFlipCheckBox_(nullptr)
    , vFlipCheckBox_(nullptr)
    , canvas_(nullptr)
    , patternList_(nullptr)
    , saveButton_(nullptr)
    , discardButton_(nullptr)
    , blockIndex_(0)
    , selectedPatternIndex_(0)
    , selectedPaletteIndex_(0)
    , dirty_(false)
{
    setModal(false);
    loadBlocks();

    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    setLayout(mainLayout);

    auto* contentLayout = new QHBoxLayout();
    auto* leftLayout = new QVBoxLayout();

    blockCombo_ = new QComboBox();
    for (size_t i = 0; i < level_->getBlockCount(); i++) {
        blockCombo_->addItem(tr("Block %1").arg(i), QVariant::fromValue(i));
    }
    leftLayout->addWidget(blockCombo_);

    auto* editorLayout = new QHBoxLayout();
    canvas_ = new BlockCanvas(this, level_, blocks_.get());
    editorLayout->addWidget(canvas_);

    auto* toolsLayout = new QVBoxLayout();
    hFlipCheckBox_ = new QCheckBox(tr("Horizontal flip"));
    vFlipCheckBox_ = new QCheckBox(tr("Vertical flip"));
    toolsLayout->addWidget(hFlipCheckBox_);
    toolsLayout->addWidget(vFlipCheckBox_);
    toolsLayout->addStretch(1);
    editorLayout->addLayout(toolsLayout);
    leftLayout->addLayout(editorLayout);
    leftLayout->addStretch(1);
    contentLayout->addLayout(leftLayout);

    patternList_ = new PatternPaletteList(this, level_);
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidget(patternList_);
    scrollArea->setWidgetResizable(false);
    scrollArea->setMinimumSize(PATTERN_LABEL_WIDTH + PATTERN_CELL_SIZE * 4 + 36, 320);
    contentLayout->addWidget(scrollArea);
    mainLayout->addLayout(contentLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    saveButton_ = new QPushButton(tr("Save"));
    discardButton_ = new QPushButton(tr("Discard"));
    auto* closeButton = new QPushButton(tr("Close"));
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(discardButton_);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(blockCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &BlockEditor::blockChanged);
    connect(hFlipCheckBox_, &QCheckBox::toggled, this, &BlockEditor::flipChanged);
    connect(vFlipCheckBox_, &QCheckBox::toggled, this, &BlockEditor::flipChanged);
    connect(canvas_, &BlockCanvas::blockModified, this, &BlockEditor::blockModified);
    connect(patternList_, &PatternPaletteList::patternSelected, this, &BlockEditor::patternSelected);
    connect(saveButton_, &QPushButton::clicked, this, &BlockEditor::saveChanges);
    connect(discardButton_, &QPushButton::clicked, this, &BlockEditor::discardChanges);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    setDirty(false);
    updateCanvasSelection();
}

BlockEditor::~BlockEditor() = default;

void BlockEditor::closeEvent(QCloseEvent* event)
{
    if (confirmDirtyChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void BlockEditor::applyBlocks()
{
    uint8_t buffer[Block::BLOCK_SIZE_IN_ROM];
    for (size_t i = 0; i < level_->getBlockCount(); i++) {
        blocks_[i].toSegaFormat(buffer);
        level_->getBlock(i).fromSegaFormat(buffer);
    }
}

bool BlockEditor::confirmDirtyChanges()
{
    if (!dirty_) {
        return true;
    }

    const auto reply = QMessageBox::warning(this,
            tr("Unsaved Blocks"),
            tr("These blocks have unsaved changes.\n\nDo you want to save them?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);

    switch (reply) {
    case QMessageBox::Save:
        saveChanges();
        return true;
    case QMessageBox::Discard:
        loadBlocks();
        canvas_->update();
        setDirty(false);
        return true;
    default:
        return false;
    }
}

void BlockEditor::loadBlocks()
{
    uint8_t buffer[Block::BLOCK_SIZE_IN_ROM];
    for (size_t i = 0; i < level_->getBlockCount(); i++) {
        level_->getBlock(i).toSegaFormat(buffer);
        blocks_[i].fromSegaFormat(buffer);
    }
}

void BlockEditor::setDirty(bool dirty)
{
    dirty_ = dirty;
    saveButton_->setEnabled(dirty);
    discardButton_->setEnabled(dirty);
    updateTitle();
}

void BlockEditor::updateCanvasSelection()
{
    canvas_->setSelectedPattern(selectedPatternIndex_,
                               selectedPaletteIndex_,
                               hFlipCheckBox_->isChecked(),
                               vFlipCheckBox_->isChecked());
}

void BlockEditor::updateTitle()
{
    setWindowTitle(tr("%1Block Editor - Block %2")
            .arg(dirty_ ? "*" : "")
            .arg(blockIndex_));
}

void BlockEditor::blockChanged(int blockIndex)
{
    if (!confirmDirtyChanges()) {
        QSignalBlocker blocker(blockCombo_);
        blockCombo_->setCurrentIndex(static_cast<int>(blockIndex_));
        return;
    }

    blockIndex_ = static_cast<size_t>(blockIndex);
    canvas_->setBlockIndex(blockIndex_);
    setDirty(false);
    updateTitle();
}

void BlockEditor::blockModified()
{
    setDirty(true);
}

void BlockEditor::discardChanges()
{
    loadBlocks();
    canvas_->update();
    setDirty(false);
}

void BlockEditor::flipChanged(int)
{
    updateCanvasSelection();
}

void BlockEditor::patternSelected(uint16_t patternIndex, uint16_t paletteIndex)
{
    selectedPatternIndex_ = patternIndex;
    selectedPaletteIndex_ = paletteIndex;
    updateCanvasSelection();
}

void BlockEditor::saveChanges()
{
    applyBlocks();
    setDirty(false);
    emit blocksModified();
}
