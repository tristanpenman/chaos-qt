#include "PatternEditor.h"


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


namespace {

constexpr int kEditorScale = 24;

QColor toQColor(const Palette::Color& color)
{
    return QColor(color.r, color.g, color.b);
}

}  // namespace

PatternCanvas::PatternCanvas(QWidget* parent, std::array<uint8_t, Pattern::kPatternSizeInMemory>& pixels)
    : QWidget(parent)
    , pixels_(pixels)
    , palette_(nullptr)
    , selectedColor_(0)
{
    setFixedSize(Pattern::kPatternWidth * kEditorScale, Pattern::kPatternHeight * kEditorScale);
    setMouseTracking(true);
}

void PatternCanvas::setPalette(const Palette* palette)
{
    palette_ = palette;
    update();
}

void PatternCanvas::setSelectedColor(uint8_t colorIndex)
{
    selectedColor_ = colorIndex;
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

    for (int y = 0; y < Pattern::kPatternHeight; y++) {
        for (int x = 0; x < Pattern::kPatternWidth; x++) {
            const QRect pixelRect(x * kEditorScale, y * kEditorScale, kEditorScale, kEditorScale);
            const auto colorIndex = pixels_[static_cast<size_t>(y) * Pattern::kPatternWidth + static_cast<size_t>(x)];
            if (palette_) {
                painter.fillRect(pixelRect, toQColor(palette_->getColor(colorIndex)));
            }
            painter.setPen(QColor(40, 40, 40));
            painter.drawRect(pixelRect.adjusted(0, 0, -1, -1));
        }
    }
}

void PatternCanvas::paintPixelAt(const QPoint& pos)
{
    const int x = pos.x() / kEditorScale;
    const int y = pos.y() / kEditorScale;
    if (x < 0 || x >= Pattern::kPatternWidth || y < 0 || y >= Pattern::kPatternHeight) {
        return;
    }

    const auto offset = static_cast<size_t>(y) * Pattern::kPatternWidth + static_cast<size_t>(x);
    if (pixels_[offset] == selectedColor_) {
        return;
    }

    pixels_[offset] = selectedColor_;
    update();
    emit patternChanged();
}

PatternEditor::PatternEditor(QWidget* parent, const std::shared_ptr<Level>& level)
    : QDialog(parent)
    , level_(level)
    , patternCombo_(nullptr)
    , paletteCombo_(nullptr)
    , colorButtons_(nullptr)
    , canvas_(nullptr)
    , preview2x_(nullptr)
    , preview4x_(nullptr)
    , preview8x_(nullptr)
    , saveButton_(nullptr)
    , discardButton_(nullptr)
    , currentPatternIndex_(0)
    , currentPaletteIndex_(0)
    , currentColorIndex_(0)
    , dirty_(false)
{
    setModal(true);

    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    setLayout(mainLayout);

    auto* selectorLayout = new QHBoxLayout();
    patternCombo_ = new QComboBox();
    for (size_t i = 0; i < level_->getPatternCount(); i++) {
        patternCombo_->addItem(tr("Pattern %1").arg(i), QVariant::fromValue(i));
    }
    selectorLayout->addWidget(patternCombo_);

    paletteCombo_ = new QComboBox();
    for (size_t i = 0; i < level_->getPaletteCount(); i++) {
        paletteCombo_->addItem(tr("Palette %1").arg(i), QVariant::fromValue(i));
    }
    selectorLayout->addWidget(paletteCombo_);
    mainLayout->addLayout(selectorLayout);

    auto* editorLayout = new QHBoxLayout();
    canvas_ = new PatternCanvas(this, pixels_);
    editorLayout->addWidget(canvas_);

    auto* paletteLayout = new QGridLayout();
    paletteLayout->setSpacing(4);
    colorButtons_ = new QButtonGroup(this);
    colorButtons_->setExclusive(true);
    for (int i = 0; i < Palette::kPaletteSize; i++) {
        auto* button = new QPushButton();
        button->setCheckable(true);
        button->setFixedSize(28, 28);
        colorButtons_->addButton(button, i);
        paletteLayout->addWidget(button, i / 4, i % 4);
    }
    editorLayout->addLayout(paletteLayout);
    mainLayout->addLayout(editorLayout);

    auto* previewLayout = new QHBoxLayout();
    preview2x_ = new QLabel();
    preview4x_ = new QLabel();
    preview8x_ = new QLabel();
    previewLayout->addWidget(preview2x_);
    previewLayout->addWidget(preview4x_);
    previewLayout->addWidget(preview8x_);
    mainLayout->addLayout(previewLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    saveButton_ = new QPushButton(tr("Save"));
    discardButton_ = new QPushButton(tr("Discard"));
    auto* closeButton = new QPushButton(tr("Close"));
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(discardButton_);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(patternCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &PatternEditor::patternChanged);
    connect(paletteCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &PatternEditor::paletteChanged);
    connect(colorButtons_, &QButtonGroup::idClicked, this, &PatternEditor::colorSelected);
    connect(canvas_, &PatternCanvas::patternChanged, this, &PatternEditor::patternEdited);
    connect(saveButton_, &QPushButton::clicked, this, &PatternEditor::saveChanges);
    connect(discardButton_, &QPushButton::clicked, this, &PatternEditor::discardChanges);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    canvas_->setPalette(&level_->getPalette(currentPaletteIndex_));
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
    Pattern& pattern = level_->getPattern(currentPatternIndex_);
    for (int y = 0; y < Pattern::kPatternHeight; y++) {
        for (int x = 0; x < Pattern::kPatternWidth; x++) {
            const auto offset = static_cast<size_t>(y) * Pattern::kPatternWidth + static_cast<size_t>(x);
            pattern.setPixel(static_cast<uint8_t>(x), static_cast<uint8_t>(y), pixels_[offset]);
        }
    }
}

bool PatternEditor::confirmDirtyChanges()
{
    if (!dirty_) {
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
    currentPatternIndex_ = patternIndex;
    const Pattern& pattern = level_->getPattern(patternIndex);
    for (int y = 0; y < Pattern::kPatternHeight; y++) {
        for (int x = 0; x < Pattern::kPatternWidth; x++) {
            const auto offset = static_cast<size_t>(y) * Pattern::kPatternWidth + static_cast<size_t>(x);
            pixels_[offset] = pattern.getPixel(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
        }
    }

    canvas_->update();
    renderPreviews();
    updateTitle();
}

void PatternEditor::populatePaletteButtons()
{
    const Palette& palette = level_->getPalette(currentPaletteIndex_);
    for (int i = 0; i < Palette::kPaletteSize; i++) {
        auto* button = colorButtons_->button(i);
        const auto color = palette.getColor(static_cast<size_t>(i));
        button->setStyleSheet(QStringLiteral("background: rgb(%1,%2,%3)").arg(color.r).arg(color.g).arg(color.b));
    }
}

void PatternEditor::renderPreview(QLabel* label, int scale)
{
    const Palette& palette = level_->getPalette(currentPaletteIndex_);
    QImage image(Pattern::kPatternWidth, Pattern::kPatternHeight, QImage::Format_RGB888);
    for (int y = 0; y < Pattern::kPatternHeight; y++) {
        for (int x = 0; x < Pattern::kPatternWidth; x++) {
            const auto offset = static_cast<size_t>(y) * Pattern::kPatternWidth + static_cast<size_t>(x);
            const auto color = palette.getColor(pixels_[offset]);
            image.setPixel(x, y, qRgb(color.r, color.g, color.b));
        }
    }

    const QSize size(Pattern::kPatternWidth * scale, Pattern::kPatternHeight * scale);
    label->setPixmap(QPixmap::fromImage(image.scaled(size, Qt::IgnoreAspectRatio, Qt::FastTransformation)));
    label->setFixedSize(size);
}

void PatternEditor::renderPreviews()
{
    renderPreview(preview2x_, 2);
    renderPreview(preview4x_, 4);
    renderPreview(preview8x_, 8);
}

void PatternEditor::setDirty(bool dirty)
{
    dirty_ = dirty;
    saveButton_->setEnabled(dirty);
    discardButton_->setEnabled(dirty);
    updateTitle();
}

void PatternEditor::updateTitle()
{
    setWindowTitle(tr("%1Pattern Editor - Pattern %2")
            .arg(dirty_ ? "*" : "")
            .arg(currentPatternIndex_));
}

void PatternEditor::colorSelected(int colorIndex)
{
    currentColorIndex_ = static_cast<uint8_t>(colorIndex);
    canvas_->setSelectedColor(currentColorIndex_);
    if (auto* button = colorButtons_->button(colorIndex)) {
        button->setChecked(true);
    }
}

void PatternEditor::discardChanges()
{
    loadPattern(currentPatternIndex_);
    setDirty(false);
}

void PatternEditor::paletteChanged(int paletteIndex)
{
    currentPaletteIndex_ = static_cast<size_t>(paletteIndex);
    canvas_->setPalette(&level_->getPalette(currentPaletteIndex_));
    populatePaletteButtons();
    renderPreviews();
}

void PatternEditor::patternChanged(int patternIndex)
{
    if (!confirmDirtyChanges()) {
        QSignalBlocker blocker(patternCombo_);
        patternCombo_->setCurrentIndex(static_cast<int>(currentPatternIndex_));
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
