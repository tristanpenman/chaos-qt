#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "../Level.h"
#include "../Palette.h"

#include "PatternEditor.h"

using namespace std;

static constexpr int EDITOR_SCALE = 24;

static QColor toQColor(const Palette::Color& color)
{
    return QColor(color.r, color.g, color.b);
}

PatternCanvas::PatternCanvas(QWidget* parent, array<uint8_t, Pattern::PATTERN_SIZE_IN_MEM>& pixels)
    : QWidget(parent)
    , m_pixels(pixels)
    , m_palette(nullptr)
    , m_selectedColor(0)
{
    setFixedSize(Pattern::PATTERN_WIDTH * EDITOR_SCALE, Pattern::PATTERN_HEIGHT * EDITOR_SCALE);
    setMouseTracking(true);
}

void PatternCanvas::setPalette(const Palette* palette)
{
    m_palette = palette;
    update();
}

void PatternCanvas::setSelectedColor(uint8_t colorIndex)
{
    m_selectedColor = colorIndex;
}

void PatternCanvas::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        paintPixelAt(event->position().toPoint());
#else
        paintPixelAt(event->pos());
#endif
    }
}

void PatternCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        paintPixelAt(event->position().toPoint());
#else
        paintPixelAt(event->pos());
#endif
    }
}

void PatternCanvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    for (int y = 0; y < Pattern::PATTERN_HEIGHT; y++) {
        for (int x = 0; x < Pattern::PATTERN_WIDTH; x++) {
            const QRect pixelRect(x * EDITOR_SCALE, y * EDITOR_SCALE, EDITOR_SCALE, EDITOR_SCALE);
            const auto colorIndex = m_pixels[static_cast<size_t>(y) * Pattern::PATTERN_WIDTH + static_cast<size_t>(x)];
            if (m_palette) {
                painter.fillRect(pixelRect, toQColor(m_palette->getColor(colorIndex)));
            }
            painter.setPen(QColor(40, 40, 40));
            painter.drawRect(pixelRect.adjusted(0, 0, -1, -1));
        }
    }
}

void PatternCanvas::paintPixelAt(const QPoint& pos)
{
    const int x = pos.x() / EDITOR_SCALE;
    const int y = pos.y() / EDITOR_SCALE;
    if (x < 0 || x >= Pattern::PATTERN_WIDTH || y < 0 || y >= Pattern::PATTERN_HEIGHT) {
        return;
    }

    const auto offset = static_cast<size_t>(y) * Pattern::PATTERN_WIDTH + static_cast<size_t>(x);
    if (m_pixels[offset] == m_selectedColor) {
        return;
    }

    m_pixels[offset] = m_selectedColor;
    update();
    emit patternChanged();
}

PatternEditor::PatternEditor(QWidget* parent, const shared_ptr<Level>& level)
    : QDialog(parent)
    , m_level(level)
    , m_patternCombo(nullptr)
    , m_paletteCombo(nullptr)
    , m_colorButtons(nullptr)
    , m_canvas(nullptr)
    , m_preview2x(nullptr)
    , m_preview4x(nullptr)
    , m_preview8x(nullptr)
    , m_saveButton(nullptr)
    , m_discardButton(nullptr)
    , m_currentPatternIndex(0)
    , m_currentPaletteIndex(0)
    , m_currentColorIndex(0)
    , m_dirty(false)
{
    setModal(true);

    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    setLayout(mainLayout);

    auto* selectorLayout = new QHBoxLayout();
    m_patternCombo = new QComboBox();
    for (size_t i = 0; i < m_level->getPatternCount(); i++) {
        m_patternCombo->addItem(QString("Pattern %1").arg(i), QVariant::fromValue(i));
    }
    selectorLayout->addWidget(m_patternCombo);

    m_paletteCombo = new QComboBox();
    for (size_t i = 0; i < m_level->getPaletteCount(); i++) {
        m_paletteCombo->addItem(QString("Palette %1").arg(i), QVariant::fromValue(i));
    }
    selectorLayout->addWidget(m_paletteCombo);
    mainLayout->addLayout(selectorLayout);

    auto* editorLayout = new QHBoxLayout();
    m_canvas = new PatternCanvas(this, m_pixels);
    editorLayout->addWidget(m_canvas);

    auto* paletteLayout = new QGridLayout();
    paletteLayout->setSpacing(4);
    m_colorButtons = new QButtonGroup(this);
    m_colorButtons->setExclusive(true);
    for (int i = 0; i < Palette::PALETTE_SIZE; i++) {
        auto* button = new QPushButton();
        button->setCheckable(true);
        button->setFixedSize(28, 28);
        m_colorButtons->addButton(button, i);
        paletteLayout->addWidget(button, i / 4, i % 4);
    }
    editorLayout->addLayout(paletteLayout);
    mainLayout->addLayout(editorLayout);

    auto* previewLayout = new QHBoxLayout();
    m_preview2x = new QLabel();
    m_preview4x = new QLabel();
    m_preview8x = new QLabel();
    previewLayout->addWidget(m_preview2x);
    previewLayout->addWidget(m_preview4x);
    previewLayout->addWidget(m_preview8x);
    mainLayout->addLayout(previewLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    m_saveButton = new QPushButton(tr("Save"));
    m_discardButton = new QPushButton(tr("Discard"));
    auto* closeButton = new QPushButton(tr("Close"));
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_discardButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_patternCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(patternChanged(int)));
    connect(m_paletteCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(paletteChanged(int)));
    connect(m_colorButtons, SIGNAL(idClicked(int)), this, SLOT(colorSelected(int)));
    connect(m_canvas, SIGNAL(patternChanged()), this, SLOT(patternEdited()));
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveChanges()));
    connect(m_discardButton, SIGNAL(clicked()), this, SLOT(discardChanges()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    m_canvas->setPalette(&m_level->getPalette(m_currentPaletteIndex));
    loadPattern(0);
    populatePaletteButtons();
    colorSelected(0);
    setDirty(false);
}

void PatternEditor::closeEvent(QCloseEvent* event)
{
    if (confirmDirtyChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void PatternEditor::applyPattern()
{
    Pattern& pattern = m_level->getPattern(m_currentPatternIndex);
    for (int y = 0; y < Pattern::PATTERN_HEIGHT; y++) {
        for (int x = 0; x < Pattern::PATTERN_WIDTH; x++) {
            const auto offset = static_cast<size_t>(y) * Pattern::PATTERN_WIDTH + static_cast<size_t>(x);
            pattern.setPixel(static_cast<uint8_t>(x), static_cast<uint8_t>(y), m_pixels[offset]);
        }
    }
}

bool PatternEditor::confirmDirtyChanges()
{
    if (!m_dirty) {
        return true;
    }

    const auto reply = QMessageBox::warning(this,
            tr("Unsaved Pattern"),
            tr("This pattern has unsaved changes.\n\nDo you want to save them?"),
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

void PatternEditor::loadPattern(size_t patternIndex)
{
    m_currentPatternIndex = patternIndex;
    const Pattern& pattern = m_level->getPattern(patternIndex);
    for (int y = 0; y < Pattern::PATTERN_HEIGHT; y++) {
        for (int x = 0; x < Pattern::PATTERN_WIDTH; x++) {
            const auto offset = static_cast<size_t>(y) * Pattern::PATTERN_WIDTH + static_cast<size_t>(x);
            m_pixels[offset] = pattern.getPixel(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
        }
    }

    m_canvas->update();
    renderPreviews();
    updateTitle();
}

void PatternEditor::populatePaletteButtons()
{
    const Palette& palette = m_level->getPalette(m_currentPaletteIndex);
    for (int i = 0; i < Palette::PALETTE_SIZE; i++) {
        auto* button = m_colorButtons->button(i);
        const auto color = palette.getColor(static_cast<size_t>(i));
        button->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(color.r).arg(color.g).arg(color.b));
    }
}

void PatternEditor::renderPreview(QLabel* label, int scale)
{
    const Palette& palette = m_level->getPalette(m_currentPaletteIndex);
    QImage image(Pattern::PATTERN_WIDTH, Pattern::PATTERN_HEIGHT, QImage::Format_RGB888);
    for (int y = 0; y < Pattern::PATTERN_HEIGHT; y++) {
        for (int x = 0; x < Pattern::PATTERN_WIDTH; x++) {
            const auto offset = static_cast<size_t>(y) * Pattern::PATTERN_WIDTH + static_cast<size_t>(x);
            const auto color = palette.getColor(m_pixels[offset]);
            image.setPixel(x, y, qRgb(color.r, color.g, color.b));
        }
    }

    const QSize size(Pattern::PATTERN_WIDTH * scale, Pattern::PATTERN_HEIGHT * scale);
    label->setPixmap(QPixmap::fromImage(image.scaled(size, Qt::IgnoreAspectRatio, Qt::FastTransformation)));
    label->setFixedSize(size);
}

void PatternEditor::renderPreviews()
{
    renderPreview(m_preview2x, 2);
    renderPreview(m_preview4x, 4);
    renderPreview(m_preview8x, 8);
}

void PatternEditor::setDirty(bool dirty)
{
    m_dirty = dirty;
    m_saveButton->setEnabled(dirty);
    m_discardButton->setEnabled(dirty);
    updateTitle();
}

void PatternEditor::updateTitle()
{
    setWindowTitle(QString("%1Pattern Editor - Pattern %2")
            .arg(m_dirty ? "*" : "")
            .arg(m_currentPatternIndex));
}

void PatternEditor::colorSelected(int colorIndex)
{
    m_currentColorIndex = static_cast<uint8_t>(colorIndex);
    m_canvas->setSelectedColor(m_currentColorIndex);
    if (auto* button = m_colorButtons->button(colorIndex)) {
        button->setChecked(true);
    }
}

void PatternEditor::discardChanges()
{
    loadPattern(m_currentPatternIndex);
    setDirty(false);
}

void PatternEditor::paletteChanged(int paletteIndex)
{
    m_currentPaletteIndex = static_cast<size_t>(paletteIndex);
    m_canvas->setPalette(&m_level->getPalette(m_currentPaletteIndex));
    populatePaletteButtons();
    renderPreviews();
}

void PatternEditor::patternChanged(int patternIndex)
{
    if (!confirmDirtyChanges()) {
        QSignalBlocker blocker(m_patternCombo);
        m_patternCombo->setCurrentIndex(static_cast<int>(m_currentPatternIndex));
        return;
    }

    loadPattern(static_cast<size_t>(patternIndex));
    setDirty(false);
}

void PatternEditor::patternEdited()
{
    renderPreviews();
    setDirty(true);
}

void PatternEditor::saveChanges()
{
    applyPattern();
    setDirty(false);
    emit patternModified();
}
