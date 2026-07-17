#include "PaletteEditor.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "../Level.h"

namespace {
uint8_t toSegaChannel(int value)
{
    return static_cast<uint8_t>((value / 0x10) * 0x10);
}

}  // namespace

PaletteEditor::PaletteEditor(QWidget* parent, const std::shared_ptr<Level>& level)
    : QDialog(parent)
    , level_(level)
    , paletteCombo_(new QComboBox())
    , colorButtons_(new QButtonGroup(this))
    , saveButton_(new QPushButton(tr("Save")))
    , discardButton_(new QPushButton(tr("Discard")))
    , paletteIndex_(0)
    , dirty_(false)
{
    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    setLayout(mainLayout);

    for (size_t i = 0; i < level_->getPaletteCount(); i++) {
        paletteCombo_->addItem(tr("Palette %1").arg(i), QVariant::fromValue(i));
    }
    mainLayout->addWidget(paletteCombo_);

    auto* paletteLayout = new QGridLayout();
    paletteLayout->setSpacing(4);
    colorButtons_->setExclusive(false);
    for (int i = 0; i < Palette::PALETTE_SIZE; i++) {
        auto* button = new QPushButton();
        button->setFixedSize(36, 36);
        button->setToolTip(tr("Colour %1").arg(i));
        colorButtons_->addButton(button, i);
        paletteLayout->addWidget(button, i / 8, i % 8);
    }
    mainLayout->addLayout(paletteLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    auto* closeButton = new QPushButton(tr("Close"));
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(discardButton_);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(paletteCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &PaletteEditor::paletteChanged);
    connect(colorButtons_, &QButtonGroup::idClicked, this, &PaletteEditor::colorClicked);
    connect(saveButton_, &QPushButton::clicked, this, &PaletteEditor::saveChanges);
    connect(discardButton_, &QPushButton::clicked, this, &PaletteEditor::discardChanges);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    loadPalette(0);
    setDirty(false);
}

void PaletteEditor::closeEvent(QCloseEvent* event)
{
    if (confirmDirtyChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool PaletteEditor::colorChanged(size_t colorIndex) const
{
    const auto& color = colors_[colorIndex];
    const auto& original = originalColors_[colorIndex];
    return color.r != original.r || color.g != original.g || color.b != original.b;
}

bool PaletteEditor::confirmDirtyChanges()
{
    if (!dirty_) {
        return true;
    }

    const auto reply = QMessageBox::warning(this,
            tr("Unsaved Palette"),
            tr("This palette has unsaved changes.\n\nDo you want to save them?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);

    switch (reply) {
    case QMessageBox::Save:
        saveChanges();
        return true;
    case QMessageBox::Discard:
        discardChanges();
        return true;
    default:
        return false;
    }
}

void PaletteEditor::loadPalette(size_t paletteIndex)
{
    paletteIndex_ = paletteIndex;
    const Palette& palette = level_->getPalette(paletteIndex);
    for (size_t i = 0; i < Palette::PALETTE_SIZE; i++) {
        colors_[i] = palette.getColor(i);
        originalColors_[i] = colors_[i];
    }
    populateColorButtons();
    updateTitle();
}

void PaletteEditor::populateColorButtons()
{
    for (size_t i = 0; i < Palette::PALETTE_SIZE; i++) {
        updateColorButton(colorButtons_->button(static_cast<int>(i)), i);
    }
}

void PaletteEditor::setDirty(bool dirty)
{
    dirty_ = dirty;
    saveButton_->setEnabled(dirty);
    discardButton_->setEnabled(dirty);
    updateTitle();
}

void PaletteEditor::updateColorButton(QAbstractButton* button, size_t colorIndex)
{
    const auto& color = colors_[colorIndex];
    const QString border = colorChanged(colorIndex)
            ? QStringLiteral("3px solid #f0a000")
            : QStringLiteral("1px solid palette(mid)");
    button->setStyleSheet(QStringLiteral("background: rgb(%1,%2,%3); border: %4")
            .arg(color.r)
            .arg(color.g)
            .arg(color.b)
            .arg(border));
}

void PaletteEditor::updateTitle()
{
    setWindowTitle(tr("%1Palette Editor - Palette %2")
            .arg(dirty_ ? "*" : "")
            .arg(paletteIndex_));
}

void PaletteEditor::colorClicked(int colorIndex)
{
    auto& color = colors_[static_cast<size_t>(colorIndex)];
    const QColor selectedColor = QColorDialog::getColor(
            QColor(color.r, color.g, color.b), this, tr("Select Colour"));
    if (!selectedColor.isValid()) {
        return;
    }

    color = Palette::Color {
        toSegaChannel(selectedColor.red()),
        toSegaChannel(selectedColor.green()),
        toSegaChannel(selectedColor.blue())
    };
    updateColorButton(colorButtons_->button(colorIndex), static_cast<size_t>(colorIndex));

    bool dirty = false;
    for (size_t i = 0; i < Palette::PALETTE_SIZE; i++) {
        dirty = dirty || colorChanged(i);
    }
    setDirty(dirty);
}

void PaletteEditor::discardChanges()
{
    loadPalette(paletteIndex_);
    setDirty(false);
}

void PaletteEditor::paletteChanged(int paletteIndex)
{
    if (!confirmDirtyChanges()) {
        QSignalBlocker blocker(paletteCombo_);
        paletteCombo_->setCurrentIndex(static_cast<int>(paletteIndex_));
        return;
    }

    loadPalette(static_cast<size_t>(paletteIndex));
    setDirty(false);
}

void PaletteEditor::saveChanges()
{
    Palette& palette = level_->getPalette(paletteIndex_);
    for (size_t i = 0; i < Palette::PALETTE_SIZE; i++) {
        palette.setColor(i, colors_[i]);
        originalColors_[i] = colors_[i];
    }
    populateColorButtons();
    setDirty(false);
    emit paletteModified();
}
