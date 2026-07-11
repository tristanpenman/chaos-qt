#pragma once

#include <array>
#include <memory>

#include <QDialog>
#include <QWidget>

#include "../Pattern.h"

class QButtonGroup;
class QCloseEvent;
class QComboBox;
class QLabel;
class QPushButton;

class Level;
class Palette;

class PatternCanvas : public QWidget
{
    Q_OBJECT

public:
    PatternCanvas(QWidget* parent, std::array<uint8_t, Pattern::PATTERN_SIZE_IN_MEM>& pixels);

    void setPalette(const Palette* palette);
    void setSelectedColor(uint8_t colorIndex);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void paintPixelAt(const QPoint& pos);

    std::array<uint8_t, Pattern::PATTERN_SIZE_IN_MEM>& m_pixels;
    const Palette* m_palette;
    uint8_t m_selectedColor;

signals:
    void patternChanged();
};

class PatternEditor : public QDialog
{
    Q_OBJECT

public:
    PatternEditor(QWidget* parent, const std::shared_ptr<Level>& level);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void applyPattern();
    bool confirmDirtyChanges();
    void loadPattern(size_t patternIndex);
    void populatePaletteButtons();
    void renderPreview(QLabel* label, int scale);
    void renderPreviews();
    void setDirty(bool dirty);
    void updateTitle();

    std::shared_ptr<Level> m_level;
    std::array<uint8_t, Pattern::PATTERN_SIZE_IN_MEM> m_pixels;

    QComboBox* m_patternCombo;
    QComboBox* m_paletteCombo;
    QButtonGroup* m_colorButtons;
    PatternCanvas* m_canvas;
    QLabel* m_preview2x;
    QLabel* m_preview4x;
    QLabel* m_preview8x;
    QPushButton* m_saveButton;
    QPushButton* m_discardButton;

    size_t m_currentPatternIndex;
    size_t m_currentPaletteIndex;
    uint8_t m_currentColorIndex;
    bool m_dirty;

private slots:
    void colorSelected(int colorIndex);
    void discardChanges();
    void paletteChanged(int paletteIndex);
    void patternChanged(int patternIndex);
    void patternEdited();
    void saveChanges();

signals:
    void patternModified();
};
