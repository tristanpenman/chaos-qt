#pragma once

#include <memory>

#include <QDialog>

class QImage;
class QLabel;
class QPixmap;

class Level;
class Palette;
class Pattern;

class PatternInspector : public QDialog
{
    Q_OBJECT

public:
    PatternInspector(QWidget* parent, const std::shared_ptr<Level>& level);

    void refresh();

private:
    void drawPattern(QImage& image, const Pattern&, const Palette&, int dx, int dy);
    void drawPatterns(size_t paletteIndex);

    std::shared_ptr<Level> level_;

    QLabel* label_;
    QPixmap* pixmap_;
    size_t paletteIndex_;

private slots:
    void paletteChanged(int);
};
