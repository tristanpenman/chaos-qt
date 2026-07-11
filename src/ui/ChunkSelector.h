#pragma once

#include <memory>

#include <QWidget>

class QGraphicsPixmapItem;
class QGraphicsScene;
class QGraphicsView;
class QLabel;

class Level;
class Rectangle;

class ChunkSelector : public QWidget
{
    Q_OBJECT

public:
    ChunkSelector(QWidget* parent, QPixmap** chunks, size_t chunkCount);

    void refresh();

protected:
    bool eventFilter(QObject* object, QEvent* ev) override;

private:
    void handleClick(const QPoint& pos);
    void handleMove(const QPoint& pos);

    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    QLabel* m_selected;
    QPixmap** m_chunks;
    QGraphicsPixmapItem** m_chunkItems;

    Rectangle* m_highlight;

    size_t m_chunkCount;

    int m_selectedChunk;
    int m_highlightedChunk;

signals:
    void chunkSelected(int);
};
