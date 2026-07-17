#pragma once

#include <memory>

#include <QDialog>

class QImage;
class QLabel;
class QPixmap;

class Chunk;
class Block;
class Level;
class Palette;
class Pattern;

class ChunkInspector : public QDialog
{
    Q_OBJECT

public:
    ChunkInspector(QWidget* parent, const std::shared_ptr<Level>& level);

    void refresh();

private:
    void drawPattern(QImage&, const Pattern&, const Palette&, int dx, int dy, bool hFlip, bool vFlip);
    void drawBlock(QImage&, const Block&, int dx, int dy, bool hFlip, bool vFlip);
    void drawChunk(size_t index);

    std::shared_ptr<Level> level_;

    QLabel* label_;
    QPixmap* pixmap_;
    size_t chunkIndex_;

private slots:
    void chunkChanged(int);
};
