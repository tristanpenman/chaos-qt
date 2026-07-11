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

    std::shared_ptr<Level> m_level;

    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    QGraphicsPixmapItem** m_tiles;
    QPixmap** m_chunks;
    ChunkSelector* m_chunkSelector;
    Rectangle* m_highlight;

    int m_highlightX;
    int m_highlightY;

    size_t m_selectedChunk;

    std::deque<std::shared_ptr<Command>> m_undoCommands;
    std::deque<std::shared_ptr<Command>> m_redoCommands;

    std::shared_ptr<PencilCommand> m_pencilCommand;

private slots:
    void chunkSelected(int);

signals:
    void currentTile(uint16_t x, uint16_t y, uint8_t value);
    void noTile();
    void undosRedosChanged(size_t undos, size_t redos);
    void mapModified();
};
