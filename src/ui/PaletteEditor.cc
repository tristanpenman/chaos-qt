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
    , m_level(level)
    , m_paletteCombo(new QComboBox())
    , m_colorButtons(new QButtonGroup(this))
    , m_saveButton(new QPushButton(tr("Save")))
    , m_discardButton(new QPushButton(tr("Discard")))
    , m_paletteIndex(0)
    , m_dirty(false)
{
    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    setLayout(mainLayout);

    for (size_t i = 0; i < m_level->getPaletteCount(); i++) {
        m_paletteCombo->addItem(tr("Palette %1").arg(i), QVariant::fromValue(i));
    }
    mainLayout->addWidget(m_paletteCombo);

    auto* paletteLayout = new QGridLayout();
    paletteLayout->setSpacing(4);
    m_colorButtons->setExclusive(false);
    for (int i = 0; i < Palette::PALETTE_SIZE; i++) {
        auto* button = new QPushButton();
        button->setFixedSize(36, 36);
        button->setToolTip(tr("Colour %1").arg(i));
        m_colorButtons->addButton(button, i);
        paletteLayout->addWidget(button, i / 8, i % 8);
    }
    mainLayout->addLayout(paletteLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    auto* closeButton = new QPushButton(tr("Close"));
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_discardButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_paletteCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &PaletteEditor::paletteChanged);
    connect(m_colorButtons, &QButtonGroup::idClicked, this, &PaletteEditor::colorClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &PaletteEditor::saveChanges);
    connect(m_discardButton, &QPushButton::clicked, this, &PaletteEditor::discardChanges);
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
    const auto& color = m_colors[colorIndex];
    const auto& original = m_originalColors[colorIndex];
    return color.r != original.r || color.g != original.g || color.b != original.b;
}

bool PaletteEditor::confirmDirtyChanges()
{
    if (!m_dirty) {
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
    m_paletteIndex = paletteIndex;
    const Palette& palette = m_level->getPalette(paletteIndex);
    for (size_t i = 0; i < Palette::PALETTE_SIZE; i++) {
        m_colors[i] = palette.getColor(i);
        m_originalColors[i] = m_colors[i];
    }
    populateColorButtons();
    updateTitle();
}

void PaletteEditor::populateColorButtons()
{
    for (size_t i = 0; i < Palette::PALETTE_SIZE; i++) {
        updateColorButton(m_colorButtons->button(static_cast<int>(i)), i);
    }
}

void PaletteEditor::setDirty(bool dirty)
{
    m_dirty = dirty;
    m_saveButton->setEnabled(dirty);
    m_discardButton->setEnabled(dirty);
    updateTitle();
}

void PaletteEditor::updateColorButton(QAbstractButton* button, size_t colorIndex)
{
    const auto& color = m_colors[colorIndex];
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
            .arg(m_dirty ? "*" : "")
            .arg(m_paletteIndex));
}

void PaletteEditor::colorClicked(int colorIndex)
{
    auto& color = m_colors[static_cast<size_t>(colorIndex)];
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
    updateColorButton(m_colorButtons->button(colorIndex), static_cast<size_t>(colorIndex));

    bool dirty = false;
    for (size_t i = 0; i < Palette::PALETTE_SIZE; i++) {
        dirty = dirty || colorChanged(i);
    }
    setDirty(dirty);
}

void PaletteEditor::discardChanges()
{
    loadPalette(m_paletteIndex);
    setDirty(false);
}

void PaletteEditor::paletteChanged(int paletteIndex)
{
    if (!confirmDirtyChanges()) {
        QSignalBlocker blocker(m_paletteCombo);
        m_paletteCombo->setCurrentIndex(static_cast<int>(m_paletteIndex));
        return;
    }

    loadPalette(static_cast<size_t>(paletteIndex));
    setDirty(false);
}

void PaletteEditor::saveChanges()
{
    Palette& palette = m_level->getPalette(m_paletteIndex);
    for (size_t i = 0; i < Palette::PALETTE_SIZE; i++) {
        palette.setColor(i, m_colors[i]);
        m_originalColors[i] = m_colors[i];
    }
    populateColorButtons();
    setDirty(false);
    emit paletteModified();
}
