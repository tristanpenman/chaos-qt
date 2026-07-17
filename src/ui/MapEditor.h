#pragma once

#include <deque>
#include <memory>

#include <QWidget>

class QGraphicsPixmapItem;
class QGraphicsScene;
class QGraphicsView;
class QPixmap;

class Block;
class ChunkSelector;
class Command;
class Level;
class Palette;
class Pattern;
class PencilCommand;
class Rectangle;

class MapEditor : public QWidget
{
    Q_OBJECT

public:
    MapEditor(QWidget* parent, const std::shared_ptr<Level>&);

    void undo();
    void redo();

    void actualSize();
    void zoomIn();
    void zoomOut();

    void drawToImage(QImage& image);
    void refreshChunks();

    int getWidth() const;
    int getHeight() const;
    size_t getSelectedChunk() const;

protected:
    bool eventFilter(QObject* object, QEvent* ev) override;

private:
    std::shared_ptr<Command> applyCommand(Command& command);

    bool handleMousePress();
    bool handleMouseRelease();

    void handleMove(const QPointF& pos);

    void drawPattern(QImage&, const Pattern&, const Palette&, int dx, int dy, bool hFlip, bool vFlip);
    void drawBlock(QImage&, const Block&, int dx, int dy, bool hFlip, bool vFlip);
    void drawChunk(QPixmap&, size_t index);

    std::shared_ptr<Level> level_;

    QGraphicsScene* scene_;
    QGraphicsView* view_;
    QGraphicsPixmapItem** tiles_;
    QPixmap** chunks_;
    ChunkSelector* chunkSelector_;
    Rectangle* highlight_;

    int highlightX_;
    int highlightY_;

    size_t selectedChunk_;

    std::deque<std::shared_ptr<Command>> undoCommands_;
    std::deque<std::shared_ptr<Command>> redoCommands_;

    std::shared_ptr<PencilCommand> pencilCommand_;

private slots:
    void chunkSelected(int);

signals:
    void currentTile(uint16_t x, uint16_t y, uint8_t value);
    void noTile();
    void undosRedosChanged(size_t undos, size_t redos);
    void mapModified();
};
