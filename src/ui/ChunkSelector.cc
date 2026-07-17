#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>
#include <QScrollEvent>
#include <QVBoxLayout>

#include "../Logger.h"

#include "Rectangle.h"

#include "ChunkSelector.h"

#undef LOG
#define LOG Logger("ChunkSelector")

static constexpr int kChunkSpacing = 5;

ChunkSelector::ChunkSelector(QWidget* parent, QPixmap** chunks, size_t chunkCount)
    : QWidget(parent)
    , scene_(nullptr)
    , view_(nullptr)
    , selected_(nullptr)
    , chunks_(chunks)
    , chunkItems_(nullptr)
    , chunkCount_(chunkCount)
    , selectedChunk_(0)
    , highlightedChunk_(-1)
{
    // add chunks to scene
    scene_ = new QGraphicsScene(this);
    chunkItems_ = new QGraphicsPixmapItem*[chunkCount];
    for (size_t i = 0; i < chunkCount; i++) {
        chunkItems_[i] = scene_->addPixmap(*chunks[i]);
        chunkItems_[i]->setPos(0, i * (128 + kChunkSpacing));
    }

    // create view
    view_ = new QGraphicsView(this);
    view_->setScene(scene_);

    // no border and light background
    view_->setFrameStyle(QFrame::NoFrame);
    view_->setStyleSheet("background: #eee");

    // disable dragging and move to first tile
    view_->setDragMode(QGraphicsView::DragMode::NoDrag);
    view_->centerOn(0, -scene_->height() / 2);

    // set width according to scrollbars...
    // TODO: not sure why height/2 works here, need to test across platforms
    const auto scrollbarSize = view_->verticalScrollBar()->height() / 2;
    view_->setFixedWidth(128 + scrollbarSize);
    view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // styleable container for selected chunk
    auto selectedContainer = new QWidget(this);
    auto selectedLayout = new QVBoxLayout(this);
    // use half-scrollbar size so that it is as wide as the scrollable view
    const auto halfScrollbarSize = scrollbarSize / 2;
    selectedLayout->setContentsMargins(halfScrollbarSize, halfScrollbarSize, halfScrollbarSize, halfScrollbarSize);
    selectedContainer->setLayout(selectedLayout);
    selectedContainer->setStyleSheet("background: #ccc");

    // selected chunk
    selected_ = new QLabel(this);
    selected_->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    selected_->setAttribute(Qt::WA_TranslucentBackground);
    selected_->setPixmap(*chunks_[0]);
    selectedLayout->addWidget(selected_);

    // layout
    auto vbox = new QVBoxLayout(this);
    vbox->addWidget(selectedContainer);
    vbox->addWidget(view_);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(8);
    setLayout(vbox);

    // highlight region
    highlight_ = new Rectangle(128, 128, QColor(128, 192, 255, 64));
    highlight_->setPos(0, 0);
    highlight_->setVisible(false);
    scene_->addItem(highlight_);

    // track mouse events
    view_->viewport()->installEventFilter(this);
    view_->setMouseTracking(true);
}

void ChunkSelector::refresh()
{
    for (size_t i = 0; i < chunkCount_; i++) {
        chunkItems_[i]->setPixmap(*chunks_[i]);
    }
    selected_->setPixmap(*chunks_[selectedChunk_]);
}

bool ChunkSelector::eventFilter(QObject* object, QEvent* ev)
{
    if (object != view_->viewport()) {
        return false;
    }

    switch (ev->type()) {
    case QEvent::Leave:
        highlightedChunk_ = -1;
        highlight_->setVisible(false);
        break;

    case QEvent::MouseButtonPress:
        handleClick(view_->viewport()->mapFromGlobal(QCursor::pos()));
        return true;

    case QEvent::MouseMove:
    case QEvent::Wheel:
        handleMove(view_->viewport()->mapFromGlobal(QCursor::pos()));
        break;

    default:
        break;
    }

    return false;
}

void ChunkSelector::handleClick(const QPoint& pos)
{
    const int y = pos.y() - kChunkSpacing + view_->verticalScrollBar()->value();
    const int tileOffset = y % (128 + kChunkSpacing);

    if (tileOffset < 128) {
        const int tile = y / (128 + kChunkSpacing);
        LOG() << "Selected chunk " << tile;
        selectedChunk_ = tile;
        selected_->setPixmap(*chunks_[tile]);
        emit chunkSelected(tile);
    }
}

void ChunkSelector::handleMove(const QPoint& pos)
{
    const int y = pos.y() - kChunkSpacing + view_->verticalScrollBar()->value();
    const int tileOffset = y % (128 + kChunkSpacing);
    if (tileOffset >= 128) {
        highlightedChunk_ = -1;
        highlight_->setVisible(false);
        return;
    }

    const int tile = y / (128 + kChunkSpacing);
    if (highlightedChunk_ != tile) {
        highlightedChunk_ = tile;
        highlight_->setPos(0, tile * (128 + kChunkSpacing));
        highlight_->setVisible(true);
    }
}
