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

    std::shared_ptr<Level> m_level;

    QLabel* m_label;
    QPixmap* m_pixmap;
    size_t m_paletteIndex;

private slots:
    void paletteChanged(int);
};
