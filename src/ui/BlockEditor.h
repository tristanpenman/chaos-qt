#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <QDialog>
#include <QPixmap>
#include <QWidget>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QPainter;
class QPushButton;

class Block;
class Level;
class Palette;
class Pattern;

class BlockCanvas : public QWidget
{
    Q_OBJECT

public:
    BlockCanvas(QWidget* parent, const std::shared_ptr<Level>& level, Block* blocks);

    void setBlockIndex(size_t blockIndex);
    void setSelectedPattern(uint16_t patternIndex, uint16_t paletteIndex, bool hFlip, bool vFlip);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void drawAt(const QPoint& pos);
    void drawBlock(QPainter& painter, const Block& block);
    void drawPattern(QPainter& painter, const Pattern& pattern, const Palette& palette, int dx, int dy, bool hFlip, bool vFlip);
    uint16_t selectedPatternDescValue() const;

    std::shared_ptr<Level> m_level;
    Block* m_blocks;
    size_t m_blockIndex;
    uint16_t m_selectedPatternIndex;
    uint16_t m_selectedPaletteIndex;
    bool m_hFlip;
    bool m_vFlip;

signals:
    void blockModified();
};

class PatternPaletteList : public QWidget
{
    Q_OBJECT

public:
    PatternPaletteList(QWidget* parent, const std::shared_ptr<Level>& level);

    void setSelected(uint16_t patternIndex, uint16_t paletteIndex);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void buildPixmapCache();
    QPixmap renderPatternPixmap(const Pattern& pattern, const Palette& palette) const;
    const QPixmap& cachedPixmap(size_t patternIndex, size_t paletteIndex) const;

    std::shared_ptr<Level> m_level;
    std::vector<QPixmap> m_pixmaps;
    uint16_t m_selectedPatternIndex;
    uint16_t m_selectedPaletteIndex;

signals:
    void patternSelected(uint16_t patternIndex, uint16_t paletteIndex);
};

class BlockEditor : public QDialog
{
    Q_OBJECT

public:
    BlockEditor(QWidget* parent, const std::shared_ptr<Level>& level);
    ~BlockEditor();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void applyBlocks();
    bool confirmDirtyChanges();
    void loadBlocks();
    void setDirty(bool dirty);
    void updateCanvasSelection();
    void updateTitle();

    std::shared_ptr<Level> m_level;
    std::unique_ptr<Block[]> m_blocks;

    QComboBox* m_blockCombo;
    QCheckBox* m_hFlipCheckBox;
    QCheckBox* m_vFlipCheckBox;
    BlockCanvas* m_canvas;
    PatternPaletteList* m_patternList;
    QPushButton* m_saveButton;
    QPushButton* m_discardButton;

    size_t m_blockIndex;
    uint16_t m_selectedPatternIndex;
    uint16_t m_selectedPaletteIndex;
    bool m_dirty;

private slots:
    void blockChanged(int blockIndex);
    void blockModified();
    void discardChanges();
    void flipChanged(int);
    void patternSelected(uint16_t patternIndex, uint16_t paletteIndex);
    void saveChanges();

signals:
    void blocksModified();
};
