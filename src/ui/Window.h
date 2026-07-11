#pragma once

#include <iosfwd>
#include <memory>

#include <QMainWindow>
#include <QStringList>

class QHBoxLayout;
class QAction;
class QMenu;
class QPushButton;
class QStatusBar;
class QCloseEvent;

class ChunkInspector;
class BlockInspector;
class Game;
class Level;
class LevelSelect;
class MapEditor;
class PaletteEditor;
class PaletteInspector;
class PatternInspector;
class Rom;
class RomInfo;
class BlockEditor;
class ChunkEditor;

class Window : public QMainWindow
{
  Q_OBJECT

public:
  Window();

  bool openRom(const QString& path);
  bool openProject(const QString& path);
  void openLevel(const QString& level);
  void exportBinary(const QString& fileName);
  void exportPng(const QString& fileName);

public slots:
  // file
  void showOpenRomDialog();
  void showOpenProjectDialog();
  void openLastRom();
  void openRecentRom();
  void showLevelSelectDialog();
  void saveRom();
  void showExportBinaryDialog();
  void showExportPngDialog();

  // edit
  void undo();
  void redo();

  // view
  void actualSize();
  void zoomIn();
  void zoomOut();

  // view/inspectors
  void showPaletteInspector();
  void showPatternInspector();
  void showBlockInspector();
  void showChunkInspector();

  // edit
  void showBlockEditor();
  void showPaletteEditor();
  void showPatternEditor();
  void showChunkEditor();

  // tools
  void showRomInfo();
  void relocateLevels();

protected:
  void closeEvent(QCloseEvent* event) override;

private slots:
  void levelSelected(int levelIdx);
  void currentTile(uint16_t x, uint16_t y, uint8_t value);
  void noTile();
  void undosRedosChanged(size_t undos, size_t redos);
  void mapModified();
  void paletteModified();
  void blocksModified();
  void patternModified();
  void chunksModified();

private:
  bool confirmCloseCurrentLevel();
  void closeCurrentLevel();
  bool openRomFromUserAction(const QString& path);
  bool openProjectFromUserAction(const QString& path);
  QStringList recentRoms() const;
  void setRecentRoms(const QStringList& paths);
  void addRecentRom(const QString& path);
  void updateRecentRomActions();
  bool trySaveRom();
  void createFileMenu();
  void createEditMenu();
  void createViewMenu();
  void createToolsMenu();
  void createMapMenu();

  // message box helpers
  static void showError(const QString& title, const QString& text);
  static void showInfo(const QString& title, const QString& text);

  // dialogs
  LevelSelect* m_levelSelect;
  PaletteInspector* m_paletteInspector;
  PatternInspector* m_patternInspector;
  BlockInspector* m_blockInspector;
  ChunkInspector* m_chunkInspector;
  RomInfo* m_romInfo;

  // editor
  MapEditor* m_mapEditor;

  // actions
  QAction* m_openRomAction;
  QAction* m_openProjectAction;
  QAction* m_levelSelectAction;
  QAction* m_saveRomAction;
  QAction* m_exportBinaryAction;
  QAction* m_exportPngAction;
  QAction* m_undoAction;
  QAction* m_redoAction;
  QAction* m_paletteEditorAction;
  QAction* m_patternEditorAction;
  QAction* m_blockEditorAction;
  QAction* m_chunkEditorAction;
  QAction* m_actualSizeAction;
  QAction* m_zoomInAction;
  QAction* m_zoomOutAction;
  QAction* m_relocateLevelsAction;
  QAction* m_romInfoAction;

  // misc
  QMenu* m_openRecentMenu;
  QMenu* m_inspectorsMenu;
  QStatusBar* m_statusBar;
  QPushButton* m_openLastRomButton;
  QPushButton* m_levelSelectButton;

  std::shared_ptr<Rom> m_rom;
  std::shared_ptr<Game> m_game;
  std::shared_ptr<Level> m_level;
  unsigned int m_levelIdx;
  bool m_hasUnsavedChanges;
};
