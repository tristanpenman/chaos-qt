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

    std::array<uint8_t, Pattern::PATTERN_SIZE_IN_MEM>& pixels_;
    const Palette* palette_;
    uint8_t selectedColor_;

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

    std::shared_ptr<Level> level_;
    std::array<uint8_t, Pattern::PATTERN_SIZE_IN_MEM> pixels_;

    QComboBox* patternCombo_;
    QComboBox* paletteCombo_;
    QButtonGroup* colorButtons_;
    PatternCanvas* canvas_;
    QLabel* preview2x_;
    QLabel* preview4x_;
    QLabel* preview8x_;
    QPushButton* saveButton_;
    QPushButton* discardButton_;

    size_t currentPatternIndex_;
    size_t currentPaletteIndex_;
    uint8_t currentColorIndex_;
    bool dirty_;

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
