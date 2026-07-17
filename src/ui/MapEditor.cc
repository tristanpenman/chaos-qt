#include "MapEditor.h"

#include <QApplication>
#include <QBrush>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QImage>
#include <QMouseEvent>
#include <QPixmap>
#include <QPen>
#include <QScrollBar>

#include "../Block.h"
#include "../Chunk.h"
#include "../Level.h"
#include "../Logger.h"
#include "../Map.h"
#include "../Palette.h"
#include "../Pattern.h"

#include "../commands/PencilCommand.h"

#include "ChunkSelector.h"
#include "Rectangle.h"
#include "ZoomSupport.h"

#undef LOG
#define LOG Logger("MapEditor")

#define MAX_UNDO_COMMANDS 20

using namespace std;

MapEditor::MapEditor(QWidget* parent, const shared_ptr<Level>& level)
    : QWidget(parent)
    , level_(level)
    , chunkSelector_(nullptr)
    , highlightX_(-1)
    , highlightY_(-1)
    , selectedChunk_(0)
{
    setStyleSheet("background: #ccc");

    // layout
    auto* hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(8, 8, 8, 8);
    hbox->setSpacing(8);
    setLayout(hbox);

    // render chunk artwork into pixmaps
    LOG() << "Drawing chunks";
    const size_t chunkCount = level_->getChunkCount();
    chunks_ = new QPixmap*[chunkCount];
    for (size_t i = 0; i < chunkCount; i++) {
        chunks_[i] = new QPixmap();
        drawChunk(*chunks_[i], i);
    }

    // populate scene
    const auto& map = level_->getMap();
    tiles_ = new QGraphicsPixmapItem*[map.getWidth() * map.getHeight()];
    scene_ = new QGraphicsScene(this);
    for (int y = 0; y < map.getHeight(); y++) {
        for (int x = 0; x < map.getWidth(); x++) {
            auto& tile = tiles_[y * map.getWidth() + x];
            tile = scene_->addPixmap(*chunks_[map.getValue(0, x, y)]);
            tile->setTransformationMode(Qt::SmoothTransformation);
            tile->setPos(x * 128, y * 128);
        }
    }

    const QPen ringPen(QColor(255, 224, 0), 2);
    const QBrush ringBrush(QColor(255, 224, 0, 48));
    for (const auto& group : level_->getRingGroups()) {
        for (uint8_t i = 0; i < group.count; i++) {
            const int x = group.x + (group.direction == RingDirection::Horizontal ? i * 0x18 : 0);
            const int y = group.y + (group.direction == RingDirection::Vertical ? i * 0x18 : 0);
            auto* ring = scene_->addRect(x - 8, y - 8, 16, 16, ringPen, ringBrush);
            ring->setToolTip(tr("Ring (%1, %2)").arg(x).arg(y));
        }
    }

    // setup scene view
    view_ = new QGraphicsView(this);
    view_->setScene(scene_);
    view_->setFrameStyle(QFrame::NoFrame);
    view_->centerOn(-scene_->width() / 2, -scene_->height() / 2);
    view_->setDragMode(QGraphicsView::DragMode::NoDrag);
    hbox->addWidget(view_);

    // highlight region
    highlight_ = new Rectangle(128, 128, QColor(128, 192, 255, 64));
    highlight_->setPos(0, 0);
    highlight_->setVisible(false);
    scene_->addItem(highlight_);

    // track mouse events
    view_->viewport()->installEventFilter(this);
    view_->setMouseTracking(true);

    // zoom support
    new ZoomSupport(view_);

    // selector
    chunkSelector_ = new ChunkSelector(this, chunks_, chunkCount);
    hbox->addWidget(chunkSelector_);
    connect(chunkSelector_, &ChunkSelector::chunkSelected, this, &MapEditor::chunkSelected);

    // allow map to grow but chunk selector remains the same size
    hbox->setStretch(0, 1);
    hbox->setStretch(1, 0);
}

void MapEditor::undo()
{
    if (undoCommands_.empty()) {
        return;
    }

    auto undoCommand = undoCommands_.front();
    undoCommands_.pop_front();

    auto redoCommand = applyCommand(*undoCommand);
    redoCommands_.push_front(redoCommand);
    if (redoCommands_.size() > MAX_UNDO_COMMANDS) {
        LOG() << "Dropping redo command";
        redoCommands_.pop_back();
    }

    emit undosRedosChanged(undoCommands_.size(), redoCommands_.size());
}

void MapEditor::redo()
{
    if (redoCommands_.empty()) {
        return;
    }

    auto redoCommand = redoCommands_.front();
    redoCommands_.pop_front();

    auto undoCommand = applyCommand(*redoCommand);
    undoCommands_.push_front(undoCommand);
    if (undoCommands_.size() > MAX_UNDO_COMMANDS) {
        LOG() << "Dropping undo command";
        undoCommands_.pop_back();
    }

    emit undosRedosChanged(undoCommands_.size(), redoCommands_.size());
}

void MapEditor::actualSize()
{
    // TODO
}

void MapEditor::zoomIn()
{
    // TODO
}

void MapEditor::zoomOut()
{
    // TODO
}

void MapEditor::drawToImage(QImage& image)
{
    QPainter painter(&image);
    scene_->render(&painter);
}

void MapEditor::refreshChunks()
{
    const size_t chunkCount = level_->getChunkCount();
    for (size_t i = 0; i < chunkCount; i++) {
        drawChunk(*chunks_[i], i);
    }
    chunkSelector_->refresh();

    const auto& map = level_->getMap();
    for (int y = 0; y < map.getHeight(); y++) {
        for (int x = 0; x < map.getWidth(); x++) {
            const auto offset = static_cast<size_t>(y) * static_cast<size_t>(map.getWidth()) + static_cast<size_t>(x);
            tiles_[offset]->setPixmap(*chunks_[map.getValue(0, x, y)]);
        }
    }
}

int MapEditor::getWidth() const
{
    return level_->getMap().getWidth() * Chunk::CHUNK_WIDTH;
}

int MapEditor::getHeight() const
{
    return level_->getMap().getHeight() * Chunk::CHUNK_HEIGHT;
}

size_t MapEditor::getSelectedChunk() const
{
    return selectedChunk_;
}

bool MapEditor::eventFilter(QObject* object, QEvent* ev)
{
    if (object != view_->viewport()) {
        return false;
    }

    switch (ev->type()) {
    case QEvent::Leave:
        highlightX_ = -1;
        highlightY_ = -1;
        highlight_->setVisible(false);
        break;

    case QEvent::MouseButtonPress:
        return handleMousePress();

    case QEvent::MouseButtonRelease:
        return handleMouseRelease();

    case QEvent::MouseMove:
        {
            auto mouseEvent = dynamic_cast<QMouseEvent*>(ev);
            handleMove(view_->mapToScene(mouseEvent->pos()));
            break;
        }

    default:
        break;
    }

    return false;
}

shared_ptr<Command> MapEditor::applyCommand(Command& command)
{
    auto result = command.commit();

    // apply changes to visible tiles
    for (const auto& change : result.changes) {
        const auto offset = static_cast<size_t>(change.y) * level_->getMap().getWidth()
            + static_cast<size_t>(change.x);
        tiles_[offset]->setPixmap(*chunks_[static_cast<size_t>(change.value)]);
    }

    return result.undoCommand;
}

bool MapEditor::handleMousePress()
{
    // TODO: Fix
    if (highlightX_ < 0 || highlightY_ < 0) {
        return false;
    }

    // update tile
    const auto offset = static_cast<size_t>(highlightY_) * level_->getMap().getWidth()
        + static_cast<size_t>(highlightX_);
    tiles_[offset]->setPixmap(*chunks_[selectedChunk_]);

    // start command
    pencilCommand_ = std::make_shared<PencilCommand>(level_->getMap());
    pencilCommand_->addChange(0, highlightX_, highlightY_, static_cast<int>(selectedChunk_));

    return true;
}

bool MapEditor::handleMouseRelease()
{
    if (!pencilCommand_) {
        return false;
    }

    // generate undo command
    const auto result = pencilCommand_->commit();
    pencilCommand_.reset();

    // save undo command
    redoCommands_.clear();
    undoCommands_.push_front(result.undoCommand);
    if (undoCommands_.size() > MAX_UNDO_COMMANDS) {
        LOG() << "Dropping undo command";
        undoCommands_.pop_back();
    }

    emit undosRedosChanged(undoCommands_.size(), redoCommands_.size());
    emit mapModified();

    return true;
}

void MapEditor::handleMove(const QPointF& pos)
{
    const int highlightX = int(pos.x() / 128);
    const int highlightY = int(pos.y() / 128);

    auto& map = level_->getMap();

    if (highlightX < 0 || highlightX >= map.getWidth() || highlightY < 0 || highlightY >= map.getHeight()) {
        highlight_->setVisible(false);
        emit noTile();
        return;
    }

    if (highlightX != highlightX_ || highlightY != highlightY_) {
        if (pencilCommand_) {
            const auto offset = static_cast<size_t>(highlightY_) * map.getWidth()
                + static_cast<size_t>(highlightX_);
            tiles_[offset]->setPixmap(*chunks_[selectedChunk_]);
            pencilCommand_->addChange(0, highlightX_, highlightY_, static_cast<int>(selectedChunk_));
        }

        highlightX_ = highlightX;
        highlightY_ = highlightY;
        highlight_->setPos(highlightX_ * 128, highlightY_ * 128);
        highlight_->setVisible(true);

        emit currentTile(highlightX, highlightY, level_->getMap().getValue(0, highlightX, highlightY));
    }
}

void MapEditor::drawPattern(QImage& image,
                            const Pattern& pattern,
                            const Palette& palette,
                            int dx,
                            int dy,
                            bool hFlip,
                            bool vFlip)
{
    for (int py = 0; py < Pattern::PATTERN_HEIGHT; py++) {
        for (int px = 0; px < Pattern::PATTERN_WIDTH; px++) {
            const auto fx = hFlip ? 7 - px : px;
            const auto fy = vFlip ? 7 - py : py;

            const auto idx = pattern.getPixel(fx, fy);
            const auto color = palette.getColor(idx);

            image.setPixel(dx + px, dy + py, qRgb(color.r, color.g, color.b));
        }
    }
}

void MapEditor::drawBlock(QImage& image, const Block& block, int dx, int dy, bool hFlip, bool vFlip)
{
    for (int py = 0; py < 2; py++) {
        for (int px = 0; px < 2; px++) {
            const auto& patternDesc = block.getPatternDesc(hFlip ? 1 - px : px, vFlip ? 1 - py : py);

            const auto paletteIndex = patternDesc.getPaletteIndex();
            const auto patternIndex = patternDesc.getPatternIndex();

            const auto& pattern = level_->getPattern(patternIndex);
            const auto& palette = level_->getPalette(paletteIndex);

            drawPattern(image,
                  pattern,
                  palette,
                  dx + px * Pattern::PATTERN_WIDTH,
                  dy + py * Pattern::PATTERN_HEIGHT,
                  patternDesc.getHFlip() ^ hFlip,
                  patternDesc.getVFlip() ^ vFlip);
        }
    }
}

void MapEditor::drawChunk(QPixmap& pixmap, size_t index)
{
    const Chunk& chunk = level_->getChunk(index);

    QImage image(Chunk::CHUNK_WIDTH, Chunk::CHUNK_HEIGHT, QImage::Format_RGB888);
    image.fill(0);

    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            const auto& blockDesc = chunk.getBlockDesc(dx, dy);
            const auto blockIndex = blockDesc.getBlockIndex();
            try {
                const auto& block = level_->getBlock(blockIndex);
                drawBlock(image, block, dx * 16, dy * 16, blockDesc.getHFlip(), blockDesc.getVFlip());
            } catch (const exception& e) {
                LOG() << "Failed to draw block: " << e.what();
            }
        }
    }

    if (!pixmap.convertFromImage(image)) {
        throw runtime_error("Failed to copy image to pixmap");
    }
}

void MapEditor::chunkSelected(int chunkIdx)
{
    selectedChunk_ = chunkIdx;
}
