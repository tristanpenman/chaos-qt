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

    QGraphicsScene* scene_;
    QGraphicsView* view_;
    QLabel* selected_;
    QPixmap** chunks_;
    QGraphicsPixmapItem** chunkItems_;

    Rectangle* highlight_;

    size_t chunkCount_;

    int selectedChunk_;
    int highlightedChunk_;

signals:
    void chunkSelected(int);
};
