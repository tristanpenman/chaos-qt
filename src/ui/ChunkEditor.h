#pragma once

#include <memory>
#include <vector>

#include <QDialog>
#include <QWidget>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPainter;
class QPushButton;

class Chunk;
class Block;
class Level;
class Palette;
class Pattern;

class ChunkCanvas : public QWidget
{
    Q_OBJECT

public:
    ChunkCanvas(QWidget* parent, const std::shared_ptr<Level>& level, Chunk* chunks);

    void setChunkIndex(size_t chunkIndex);
    void setSelectedBlock(uint16_t blockIndex);
    void setHorizontalFlip(bool enabled);
    void setVerticalFlip(bool enabled);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void drawAt(const QPoint& pos);
    void drawChunk(QPainter& painter, const Chunk& chunk);
    void drawBlock(QPainter& painter, const Block& block, int dx, int dy, bool hFlip, bool vFlip);
    void drawPattern(QPainter& painter, const Pattern& pattern, const Palette& palette, int dx, int dy, bool hFlip, bool vFlip);

    std::shared_ptr<Level> m_level;
    Chunk* m_chunks;
    size_t m_chunkIndex;
    uint16_t m_selectedBlockIndex;
    bool m_hFlip;
    bool m_vFlip;
    int m_highlightX;
    int m_highlightY;

signals:
    void chunkModified();
};

class ChunkEditor : public QDialog
{
    Q_OBJECT

public:
    ChunkEditor(QWidget* parent, const std::shared_ptr<Level>& level, size_t initialChunkIndex);
    ~ChunkEditor();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void applyChunks();
    bool confirmDirtyChanges();
    void loadChunks();
    QPixmap renderBlockPreview(size_t blockIndex, int scale) const;
    void drawPattern(QImage& image, const Pattern& pattern, const Palette& palette, int dx, int dy, bool hFlip, bool vFlip) const;
    void drawBlockPreview(QImage& image, const Block& block, int dx, int dy) const;
    void populateBlockSelector();
    void setDirty(bool dirty);
    void updateTitle();

    std::shared_ptr<Level> m_level;
    std::unique_ptr<Chunk[]> m_chunks;

    QComboBox* m_chunkCombo;
    QListWidget* m_blockList;
    QCheckBox* m_hFlipCheckBox;
    QCheckBox* m_vFlipCheckBox;
    ChunkCanvas* m_canvas;
    QPushButton* m_saveButton;
    QPushButton* m_discardButton;

    size_t m_chunkIndex;
    bool m_dirty;

private slots:
    void blockChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void discardChanges();
    void horizontalFlipChanged(int state);
    void chunkChanged(int chunkIndex);
    void chunkModified();
    void saveChanges();
    void verticalFlipChanged(int state);

signals:
    void chunksModified();
};
