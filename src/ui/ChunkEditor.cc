#include "ChunkEditor.h"


#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include "../Chunk.h"
#include "../Block.h"
#include "../Level.h"
#include "../Palette.h"
#include "../Pattern.h"


namespace {

constexpr int kCanvasScale = 3;
constexpr int kBlockPreviewScale = 2;
constexpr uint16_t kHorizontalFlipMask = 0x400;
constexpr uint16_t kVerticalFlipMask = 0x800;

QColor toQColor(const Palette::Color& color)
{
    return QColor(color.r, color.g, color.b);
}

}  // namespace

ChunkCanvas::ChunkCanvas(QWidget* parent, const std::shared_ptr<Level>& level, Chunk* chunks)
    : QWidget(parent)
    , level_(level)
    , chunks_(chunks)
    , chunkIndex_(0)
    , selectedBlockIndex_(0)
    , hFlip_(false)
    , vFlip_(false)
    , highlightX_(-1)
    , highlightY_(-1)
{
    setFixedSize(Chunk::kChunkWidth * kCanvasScale, Chunk::kChunkHeight * kCanvasScale);
    setMouseTracking(true);
}

void ChunkCanvas::setChunkIndex(size_t chunkIndex)
{
    chunkIndex_ = chunkIndex;
    update();
}

void ChunkCanvas::setSelectedBlock(uint16_t blockIndex)
{
    selectedBlockIndex_ = blockIndex;
}

void ChunkCanvas::setHorizontalFlip(bool enabled)
{
    hFlip_ = enabled;
}

void ChunkCanvas::setVerticalFlip(bool enabled)
{
    vFlip_ = enabled;
}

void ChunkCanvas::mouseMoveEvent(QMouseEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPoint pos = event->position().toPoint();
#else
    const QPoint pos = event->pos();
#endif
    highlightX_ = pos.x() / (Block::kBlockWidth * kCanvasScale);
    highlightY_ = pos.y() / (Block::kBlockHeight * kCanvasScale);
    if (highlightX_ < 0 || highlightX_ >= 8 || highlightY_ < 0 || highlightY_ >= 8) {
        highlightX_ = -1;
        highlightY_ = -1;
    }

    if (event->buttons() & Qt::LeftButton) {
        drawAt(pos);
    } else {
        update();
    }
}

void ChunkCanvas::mousePressEvent(QMouseEvent* event)
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

void ChunkCanvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.scale(kCanvasScale, kCanvasScale);
    painter.fillRect(QRect(0, 0, Chunk::kChunkWidth, Chunk::kChunkHeight), Qt::black);

    drawChunk(painter, chunks_[chunkIndex_]);

    painter.setPen(QColor(55, 55, 55));
    for (int i = 0; i <= 8; i++) {
        painter.drawLine(i * Block::kBlockWidth, 0, i * Block::kBlockWidth, Chunk::kChunkHeight);
        painter.drawLine(0, i * Block::kBlockHeight, Chunk::kChunkWidth, i * Block::kBlockHeight);
    }

    if (highlightX_ >= 0 && highlightY_ >= 0) {
        painter.fillRect(highlightX_ * Block::kBlockWidth,
                     highlightY_ * Block::kBlockHeight,
                     Block::kBlockWidth,
                     Block::kBlockHeight,
                     QColor(128, 192, 255, 64));
    }
}

void ChunkCanvas::drawAt(const QPoint& pos)
{
    const int x = pos.x() / (Block::kBlockWidth * kCanvasScale);
    const int y = pos.y() / (Block::kBlockHeight * kCanvasScale);
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return;
    }

    uint16_t value = selectedBlockIndex_ & 0x3FF;
    if (hFlip_) {
        value |= kHorizontalFlipMask;
    }
    if (vFlip_) {
        value |= kVerticalFlipMask;
    }

    Chunk& chunk = chunks_[chunkIndex_];
    if (chunk.getBlockDesc(static_cast<uint8_t>(x), static_cast<uint8_t>(y)).get() == value) {
        return;
    }

    chunk.setBlockDesc(static_cast<uint8_t>(x), static_cast<uint8_t>(y), value);
    update();
    emit chunkModified();
}

void ChunkCanvas::drawChunk(QPainter& painter, const Chunk& chunk)
{
    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            const auto& blockDesc = chunk.getBlockDesc(static_cast<uint8_t>(dx), static_cast<uint8_t>(dy));
            try {
                const auto& block = level_->getBlock(blockDesc.getBlockIndex());
                drawBlock(painter, block, dx * Block::kBlockWidth, dy * Block::kBlockHeight, blockDesc.getHFlip(), blockDesc.getVFlip());
            } catch (...) {
            }
        }
    }
}

void ChunkCanvas::drawBlock(QPainter& painter, const Block& block, int dx, int dy, bool hFlip, bool vFlip)
{
    for (int py = 0; py < 2; py++) {
        for (int px = 0; px < 2; px++) {
            const auto& patternDesc = block.getPatternDesc(hFlip ? 1 - px : px, vFlip ? 1 - py : py);
            const auto& pattern = level_->getPattern(patternDesc.getPatternIndex());
            const auto& palette = level_->getPalette(patternDesc.getPaletteIndex());
            drawPattern(painter,
                  pattern,
                  palette,
                  dx + px * Pattern::kPatternWidth,
                  dy + py * Pattern::kPatternHeight,
                  patternDesc.getHFlip() ^ hFlip,
                  patternDesc.getVFlip() ^ vFlip);
        }
    }
}

void ChunkCanvas::drawPattern(QPainter& painter,
                             const Pattern& pattern,
                             const Palette& palette,
                             int dx,
                             int dy,
                             bool hFlip,
                             bool vFlip)
{
    for (int py = 0; py < Pattern::kPatternHeight; py++) {
        for (int px = 0; px < Pattern::kPatternWidth; px++) {
            const auto fx = hFlip ? Pattern::kPatternWidth - 1 - px : px;
            const auto fy = vFlip ? Pattern::kPatternHeight - 1 - py : py;
            const auto color = palette.getColor(pattern.getPixel(static_cast<uint8_t>(fx), static_cast<uint8_t>(fy)));
            painter.fillRect(dx + px, dy + py, 1, 1, toQColor(color));
        }
    }
}

ChunkEditor::ChunkEditor(QWidget* parent, const std::shared_ptr<Level>& level, size_t initialChunkIndex)
    : QDialog(parent)
    , level_(level)
    , chunks_(new Chunk[level->getChunkCount()])
    , chunkCombo_(nullptr)
    , blockList_(nullptr)
    , hFlipCheckBox_(nullptr)
    , vFlipCheckBox_(nullptr)
    , canvas_(nullptr)
    , saveButton_(nullptr)
    , discardButton_(nullptr)
    , chunkIndex_(initialChunkIndex < level->getChunkCount() ? initialChunkIndex : 0)
    , dirty_(false)
{
    setModal(false);
    loadChunks();

    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    setLayout(mainLayout);

    auto* selectorLayout = new QHBoxLayout();
    chunkCombo_ = new QComboBox();
    for (size_t i = 0; i < level_->getChunkCount(); i++) {
        chunkCombo_->addItem(tr("Chunk %1").arg(i), QVariant::fromValue(i));
    }
    selectorLayout->addWidget(chunkCombo_);
    mainLayout->addLayout(selectorLayout);

    auto* editorLayout = new QHBoxLayout();
    canvas_ = new ChunkCanvas(this, level_, chunks_.get());
    canvas_->setChunkIndex(chunkIndex_);
    editorLayout->addWidget(canvas_);

    auto* toolsLayout = new QVBoxLayout();
    blockList_ = new QListWidget();
    blockList_->setIconSize(QSize(Block::kBlockWidth * kBlockPreviewScale, Block::kBlockHeight * kBlockPreviewScale));
    blockList_->setMinimumWidth(170);
    populateBlockSelector();
    toolsLayout->addWidget(blockList_);

    hFlipCheckBox_ = new QCheckBox(tr("Horizontal flip"));
    vFlipCheckBox_ = new QCheckBox(tr("Vertical flip"));
    toolsLayout->addWidget(hFlipCheckBox_);
    toolsLayout->addWidget(vFlipCheckBox_);
    editorLayout->addLayout(toolsLayout);
    mainLayout->addLayout(editorLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    saveButton_ = new QPushButton(tr("Save"));
    discardButton_ = new QPushButton(tr("Discard"));
    auto* closeButton = new QPushButton(tr("Close"));
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(discardButton_);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(chunkCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &ChunkEditor::chunkChanged);
    connect(blockList_, &QListWidget::currentItemChanged, this, &ChunkEditor::blockChanged);
    connect(hFlipCheckBox_, &QCheckBox::toggled, this, &ChunkEditor::horizontalFlipChanged);
    connect(vFlipCheckBox_, &QCheckBox::toggled, this, &ChunkEditor::verticalFlipChanged);
    connect(canvas_, &ChunkCanvas::chunkModified, this, &ChunkEditor::chunkModified);
    connect(saveButton_, &QPushButton::clicked, this, &ChunkEditor::saveChanges);
    connect(discardButton_, &QPushButton::clicked, this, &ChunkEditor::discardChanges);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    chunkCombo_->setCurrentIndex(static_cast<int>(chunkIndex_));
    if (blockList_->count() > 0) {
        blockList_->setCurrentRow(0);
    }
    setDirty(false);
}

ChunkEditor::~ChunkEditor() = default;

void ChunkEditor::closeEvent(QCloseEvent* event)
{
    if (confirmDirtyChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void ChunkEditor::applyChunks()
{
    uint8_t buffer[Chunk::kChunkSizeInRom];
    for (size_t i = 0; i < level_->getChunkCount(); i++) {
        chunks_[i].toSegaFormat(buffer);
        level_->getChunk(i).fromSegaFormat(buffer);
    }
}

bool ChunkEditor::confirmDirtyChanges()
{
    if (!dirty_) {
        return true;
    }

    const auto reply = QMessageBox::warning(this,
            tr("Unsaved Chunks"),
            tr("These chunks have unsaved changes.\n\nDo you want to save them?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);

    switch (reply) {
    case QMessageBox::Save:
        saveChanges();
        return true;
    case QMessageBox::Discard:
        setDirty(false);
        return true;
    default:
        return false;
    }
}

void ChunkEditor::loadChunks()
{
    uint8_t buffer[Chunk::kChunkSizeInRom];
    for (size_t i = 0; i < level_->getChunkCount(); i++) {
        level_->getChunk(i).toSegaFormat(buffer);
        chunks_[i].fromSegaFormat(buffer);
    }
}

QPixmap ChunkEditor::renderBlockPreview(size_t blockIndex, int scale) const
{
    QImage image(Block::kBlockWidth, Block::kBlockHeight, QImage::Format_RGB888);
    image.fill(Qt::black);

    try {
        drawBlockPreview(image, level_->getBlock(blockIndex), 0, 0);
    } catch (...) {
    }

    return QPixmap::fromImage(image.scaled(Block::kBlockWidth * scale,
                                         Block::kBlockHeight * scale,
                                         Qt::IgnoreAspectRatio,
                                         Qt::FastTransformation));
}

void ChunkEditor::drawPattern(QImage& image,
                             const Pattern& pattern,
                             const Palette& palette,
                             int dx,
                             int dy,
                             bool hFlip,
                             bool vFlip) const
{
    for (int py = 0; py < Pattern::kPatternHeight; py++) {
        for (int px = 0; px < Pattern::kPatternWidth; px++) {
            const auto fx = hFlip ? Pattern::kPatternWidth - 1 - px : px;
            const auto fy = vFlip ? Pattern::kPatternHeight - 1 - py : py;
            const auto color = palette.getColor(pattern.getPixel(static_cast<uint8_t>(fx), static_cast<uint8_t>(fy)));
            image.setPixel(dx + px, dy + py, qRgb(color.r, color.g, color.b));
        }
    }
}

void ChunkEditor::drawBlockPreview(QImage& image, const Block& block, int dx, int dy) const
{
    for (int py = 0; py < 2; py++) {
        for (int px = 0; px < 2; px++) {
            const auto& patternDesc = block.getPatternDesc(static_cast<uint8_t>(px), static_cast<uint8_t>(py));
            const auto& pattern = level_->getPattern(patternDesc.getPatternIndex());
            const auto& palette = level_->getPalette(patternDesc.getPaletteIndex());
            drawPattern(image,
                  pattern,
                  palette,
                  dx + px * Pattern::kPatternWidth,
                  dy + py * Pattern::kPatternHeight,
                  patternDesc.getHFlip(),
                  patternDesc.getVFlip());
        }
    }
}

void ChunkEditor::populateBlockSelector()
{
    blockList_->clear();
    for (size_t i = 0; i < level_->getBlockCount(); i++) {
        auto* item = new QListWidgetItem(QIcon(renderBlockPreview(i, kBlockPreviewScale)), tr("Block %1").arg(i));
        item->setData(Qt::UserRole, QVariant::fromValue(static_cast<unsigned int>(i)));
        blockList_->addItem(item);
    }
}

void ChunkEditor::setDirty(bool dirty)
{
    dirty_ = dirty;
    saveButton_->setEnabled(dirty);
    discardButton_->setEnabled(dirty);
    updateTitle();
}

void ChunkEditor::updateTitle()
{
    setWindowTitle(tr("%1Chunk Editor - Chunk %2")
            .arg(dirty_ ? "*" : "")
            .arg(chunkIndex_));
}

void ChunkEditor::blockChanged(QListWidgetItem* current, QListWidgetItem*)
{
    if (!current) {
        return;
    }

    canvas_->setSelectedBlock(static_cast<uint16_t>(current->data(Qt::UserRole).toUInt()));
}

void ChunkEditor::discardChanges()
{
    loadChunks();
    canvas_->update();
    setDirty(false);
}

void ChunkEditor::horizontalFlipChanged(int state)
{
    canvas_->setHorizontalFlip(state == Qt::Checked);
}

void ChunkEditor::chunkChanged(int chunkIndex)
{
    chunkIndex_ = static_cast<size_t>(chunkIndex);
    canvas_->setChunkIndex(chunkIndex_);
    updateTitle();
}

void ChunkEditor::chunkModified()
{
    setDirty(true);
}

void ChunkEditor::saveChanges()
{
    applyChunks();
    setDirty(false);
    emit chunksModified();
}

void ChunkEditor::verticalFlipChanged(int state)
{
    canvas_->setVerticalFlip(state == Qt::Checked);
}
