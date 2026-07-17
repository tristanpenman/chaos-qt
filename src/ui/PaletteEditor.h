#pragma once

#include <array>
#include <memory>

#include <QDialog>

#include "../Palette.h"

class QAbstractButton;
class QButtonGroup;
class QCloseEvent;
class QComboBox;
class QPushButton;

class Level;

class PaletteEditor : public QDialog
{
    Q_OBJECT

public:
    PaletteEditor(QWidget* parent, const std::shared_ptr<Level>& level);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    bool colorChanged(size_t colorIndex) const;
    bool confirmDirtyChanges();
    void loadPalette(size_t paletteIndex);
    void populateColorButtons();
    void setDirty(bool dirty);
    void updateColorButton(QAbstractButton* button, size_t colorIndex);
    void updateTitle();

    std::shared_ptr<Level> level_;
    std::array<Palette::Color, Palette::kPaletteSize> colors_;
    std::array<Palette::Color, Palette::kPaletteSize> originalColors_;

    QComboBox* paletteCombo_;
    QButtonGroup* colorButtons_;
    QPushButton* saveButton_;
    QPushButton* discardButton_;

    size_t paletteIndex_;
    bool dirty_;

private slots:
    void colorClicked(int colorIndex);
    void discardChanges();
    void paletteChanged(int paletteIndex);
    void saveChanges();

signals:
    void paletteModified();
};
