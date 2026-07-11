#pragma once

#include <memory>

#include <QDialog>

class QImage;
class QLabel;
class QPixmap;

class Block;
class Level;
class Palette;
class Pattern;

class BlockInspector : public QDialog
{
    Q_OBJECT

public:
    BlockInspector(QWidget* parent, const std::shared_ptr<Level>& level);

    void refresh();

private:
    void drawPattern(QImage&, const Pattern&, const Palette&, int dx, int dy, bool hFlip, bool vFlip);
    void drawBlock(QImage&, const Block&, int dx, int dy);
    void drawBlocks();

    std::shared_ptr<Level> m_level;

    QLabel* m_label;
    QPixmap* m_pixmap;
};
