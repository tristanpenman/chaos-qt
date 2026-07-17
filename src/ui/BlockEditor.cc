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
    , m_level(level)
    , m_blocks(blocks)
    , m_blockIndex(0)
    , m_selectedPatternIndex(0)
    , m_selectedPaletteIndex(0)
    , m_hFlip(false)
    , m_vFlip(false)
{
    setFixedSize(Block::BLOCK_WIDTH * CANVAS_SCALE, Block::BLOCK_HEIGHT * CANVAS_SCALE);
}

void BlockCanvas::setBlockIndex(size_t blockIndex)
{
    m_blockIndex = blockIndex;
    update();
}

void BlockCanvas::setSelectedPattern(uint16_t patternIndex, uint16_t paletteIndex, bool hFlip, bool vFlip)
{
    m_selectedPatternIndex = patternIndex;
    m_selectedPaletteIndex = paletteIndex;
    m_hFlip = hFlip;
    m_vFlip = vFlip;
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

    drawBlock(painter, m_blocks[m_blockIndex]);

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

    Block& block = m_blocks[m_blockIndex];
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
            const auto& pattern = m_level->getPattern(patternDesc.getPatternIndex());
            const auto& palette = m_level->getPalette(patternDesc.getPaletteIndex());
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
    uint16_t value = m_selectedPatternIndex & 0x7FF;
    value |= (m_selectedPaletteIndex & 0x3) << 13;
    if (m_hFlip) {
        value |= H_FLIP_MASK;
    }
    if (m_vFlip) {
        value |= V_FLIP_MASK;
    }
    return value;
}

PatternPaletteList::PatternPaletteList(QWidget* parent, const shared_ptr<Level>& level)
    : QWidget(parent)
    , m_level(level)
    , m_selectedPatternIndex(0)
    , m_selectedPaletteIndex(0)
{
    setMinimumWidth(PATTERN_LABEL_WIDTH + PATTERN_CELL_SIZE * 4 + 16);
    setFixedHeight(static_cast<int>(m_level->getPatternCount()) * PATTERN_ROW_HEIGHT);
    buildPixmapCache();
}

void PatternPaletteList::setSelected(uint16_t patternIndex, uint16_t paletteIndex)
{
    m_selectedPatternIndex = patternIndex;
    m_selectedPaletteIndex = paletteIndex;
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
    if (patternIndex < 0 || patternIndex >= static_cast<int>(m_level->getPatternCount()) ||
            paletteIndex < 0 || paletteIndex >= 4) {
        return;
    }

    m_selectedPatternIndex = static_cast<uint16_t>(patternIndex);
    m_selectedPaletteIndex = static_cast<uint16_t>(paletteIndex);
    update();
    emit patternSelected(m_selectedPatternIndex, m_selectedPaletteIndex);
}

void PatternPaletteList::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    painter.setRenderHint(QPainter::Antialiasing, false);

    for (size_t patternIndex = 0; patternIndex < m_level->getPatternCount(); patternIndex++) {
        const int y = static_cast<int>(patternIndex) * PATTERN_ROW_HEIGHT;
        painter.setPen(palette().text().color());
        painter.drawText(QRect(0, y, PATTERN_LABEL_WIDTH - 8, PATTERN_ROW_HEIGHT), Qt::AlignVCenter | Qt::AlignRight, tr("Pattern %1").arg(patternIndex));

        for (size_t paletteIndex = 0; paletteIndex < 4; paletteIndex++) {
            const int x = PATTERN_LABEL_WIDTH + static_cast<int>(paletteIndex) * PATTERN_CELL_SIZE;
            painter.drawPixmap(x, y + 3, cachedPixmap(patternIndex, paletteIndex));

            if (patternIndex == m_selectedPatternIndex && paletteIndex == m_selectedPaletteIndex) {
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
    m_pixmaps.clear();
    m_pixmaps.reserve(m_level->getPatternCount() * 4);
    for (size_t patternIndex = 0; patternIndex < m_level->getPatternCount(); patternIndex++) {
        for (size_t paletteIndex = 0; paletteIndex < 4; paletteIndex++) {
            m_pixmaps.push_back(renderPatternPixmap(m_level->getPattern(patternIndex), m_level->getPalette(paletteIndex)));
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
    return m_pixmaps[patternIndex * 4 + paletteIndex];
}

BlockEditor::BlockEditor(QWidget* parent, const shared_ptr<Level>& level)
    : QDialog(parent)
    , m_level(level)
    , m_blocks(new Block[level->getBlockCount()])
    , m_blockCombo(nullptr)
    , m_hFlipCheckBox(nullptr)
    , m_vFlipCheckBox(nullptr)
    , m_canvas(nullptr)
    , m_patternList(nullptr)
    , m_saveButton(nullptr)
    , m_discardButton(nullptr)
    , m_blockIndex(0)
    , m_selectedPatternIndex(0)
    , m_selectedPaletteIndex(0)
    , m_dirty(false)
{
    setModal(false);
    loadBlocks();

    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    setLayout(mainLayout);

    auto* contentLayout = new QHBoxLayout();
    auto* leftLayout = new QVBoxLayout();

    m_blockCombo = new QComboBox();
    for (size_t i = 0; i < m_level->getBlockCount(); i++) {
        m_blockCombo->addItem(tr("Block %1").arg(i), QVariant::fromValue(i));
    }
    leftLayout->addWidget(m_blockCombo);

    auto* editorLayout = new QHBoxLayout();
    m_canvas = new BlockCanvas(this, m_level, m_blocks.get());
    editorLayout->addWidget(m_canvas);

    auto* toolsLayout = new QVBoxLayout();
    m_hFlipCheckBox = new QCheckBox(tr("Horizontal flip"));
    m_vFlipCheckBox = new QCheckBox(tr("Vertical flip"));
    toolsLayout->addWidget(m_hFlipCheckBox);
    toolsLayout->addWidget(m_vFlipCheckBox);
    toolsLayout->addStretch(1);
    editorLayout->addLayout(toolsLayout);
    leftLayout->addLayout(editorLayout);
    leftLayout->addStretch(1);
    contentLayout->addLayout(leftLayout);

    m_patternList = new PatternPaletteList(this, m_level);
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidget(m_patternList);
    scrollArea->setWidgetResizable(false);
    scrollArea->setMinimumSize(PATTERN_LABEL_WIDTH + PATTERN_CELL_SIZE * 4 + 36, 320);
    contentLayout->addWidget(scrollArea);
    mainLayout->addLayout(contentLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    m_saveButton = new QPushButton(tr("Save"));
    m_discardButton = new QPushButton(tr("Discard"));
    auto* closeButton = new QPushButton(tr("Close"));
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_discardButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_blockCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &BlockEditor::blockChanged);
    connect(m_hFlipCheckBox, &QCheckBox::toggled, this, &BlockEditor::flipChanged);
    connect(m_vFlipCheckBox, &QCheckBox::toggled, this, &BlockEditor::flipChanged);
    connect(m_canvas, &BlockCanvas::blockModified, this, &BlockEditor::blockModified);
    connect(m_patternList, &PatternPaletteList::patternSelected, this, &BlockEditor::patternSelected);
    connect(m_saveButton, &QPushButton::clicked, this, &BlockEditor::saveChanges);
    connect(m_discardButton, &QPushButton::clicked, this, &BlockEditor::discardChanges);
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
    for (size_t i = 0; i < m_level->getBlockCount(); i++) {
        m_blocks[i].toSegaFormat(buffer);
        m_level->getBlock(i).fromSegaFormat(buffer);
    }
}

bool BlockEditor::confirmDirtyChanges()
{
    if (!m_dirty) {
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
        m_canvas->update();
        setDirty(false);
        return true;
    default:
        return false;
    }
}

void BlockEditor::loadBlocks()
{
    uint8_t buffer[Block::BLOCK_SIZE_IN_ROM];
    for (size_t i = 0; i < m_level->getBlockCount(); i++) {
        m_level->getBlock(i).toSegaFormat(buffer);
        m_blocks[i].fromSegaFormat(buffer);
    }
}

void BlockEditor::setDirty(bool dirty)
{
    m_dirty = dirty;
    m_saveButton->setEnabled(dirty);
    m_discardButton->setEnabled(dirty);
    updateTitle();
}

void BlockEditor::updateCanvasSelection()
{
    m_canvas->setSelectedPattern(m_selectedPatternIndex,
                               m_selectedPaletteIndex,
                               m_hFlipCheckBox->isChecked(),
                               m_vFlipCheckBox->isChecked());
}

void BlockEditor::updateTitle()
{
    setWindowTitle(tr("%1Block Editor - Block %2")
            .arg(m_dirty ? "*" : "")
            .arg(m_blockIndex));
}

void BlockEditor::blockChanged(int blockIndex)
{
    if (!confirmDirtyChanges()) {
        QSignalBlocker blocker(m_blockCombo);
        m_blockCombo->setCurrentIndex(static_cast<int>(m_blockIndex));
        return;
    }

    m_blockIndex = static_cast<size_t>(blockIndex);
    m_canvas->setBlockIndex(m_blockIndex);
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
    m_canvas->update();
    setDirty(false);
}

void BlockEditor::flipChanged(int)
{
    updateCanvasSelection();
}

void BlockEditor::patternSelected(uint16_t patternIndex, uint16_t paletteIndex)
{
    m_selectedPatternIndex = patternIndex;
    m_selectedPaletteIndex = paletteIndex;
    updateCanvasSelection();
}

void BlockEditor::saveChanges()
{
    applyBlocks();
    setDirty(false);
    emit blocksModified();
}
