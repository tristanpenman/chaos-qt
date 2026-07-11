#include <iostream>

#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QStatusBar>
#include <QVBoxLayout>

#include "../Game.h"
#include "../GameFactory.h"
#include "../Level.h"
#include "../Logger.h"
#include "../Map.h"
#include "../Rom.h"

#include "BlockEditor.h"
#include "ChunkInspector.h"
#include "BlockInspector.h"
#include "LevelSelect.h"
#include "MapEditor.h"
#include "PaletteEditor.h"
#include "PaletteInspector.h"
#include "PatternEditor.h"
#include "PatternInspector.h"
#include "RomInfo.h"
#include "ChunkEditor.h"

#include "Window.h"

#undef LOG
#define LOG Logger("Window")

using namespace std;

namespace {
constexpr int MaxRecentRoms = 10;
constexpr const char* SettingsOrganization = "Tristan Penman";
constexpr const char* SettingsApplication = "SpinDash";
constexpr const char* RecentRomsKey = "recentRoms";
}  // namespace

Window::Window()
  : QMainWindow(nullptr)
  , m_levelSelect(nullptr)
  , m_paletteInspector(nullptr)
  , m_patternInspector(nullptr)
  , m_blockInspector(nullptr)
  , m_chunkInspector(nullptr)
  , m_romInfo(nullptr)
  , m_mapEditor(nullptr)
  , m_openRomAction(nullptr)
  , m_openProjectAction(nullptr)
  , m_levelSelectAction(nullptr)
  , m_saveRomAction(nullptr)
  , m_exportBinaryAction(nullptr)
  , m_exportPngAction(nullptr)
  , m_undoAction(nullptr)
  , m_redoAction(nullptr)
  , m_paletteEditorAction(nullptr)
  , m_patternEditorAction(nullptr)
  , m_blockEditorAction(nullptr)
  , m_chunkEditorAction(nullptr)
  , m_actualSizeAction(nullptr)
  , m_zoomInAction(nullptr)
  , m_zoomOutAction(nullptr)
  , m_relocateLevelsAction(nullptr)
  , m_romInfoAction(nullptr)
  , m_openRecentMenu(nullptr)
  , m_inspectorsMenu(nullptr)
  , m_statusBar(nullptr)
  , m_openLastRomButton(nullptr)
  , m_rom(nullptr)
  , m_game(nullptr)
  , m_level(nullptr)
  , m_levelIdx(0)
  , m_hasUnsavedChanges(false)
{
  setWindowTitle("SpinDash");
  setMinimumSize(320, 240);
  setAttribute(Qt::WA_AcceptTouchEvents, false);

  // choose a nice default width and height, and center the window
  const auto geometry = QGuiApplication::primaryScreen()->geometry();
  const int width = int(geometry.height() * 0.75);
  const int height = int(geometry.height() * 0.5);
  setGeometry(0, 0, width, height);
  move(geometry.center() - rect().center());

  // menus
  createFileMenu();
  createEditMenu();
  createViewMenu();
  createToolsMenu();
  createMapMenu();

  // statusbar
  m_statusBar = new QStatusBar(this);
  m_statusBar->showMessage(tr("Ready"));
  setStatusBar(m_statusBar);

  // open rom button
  const auto openRomButton = new QPushButton(tr("Open ROM..."));
  openRomButton->setMaximumWidth(250);
  connect(openRomButton, SIGNAL(clicked()), this, SLOT(showOpenRomDialog()));

  m_openLastRomButton = new QPushButton(tr("Open Last..."));
  m_openLastRomButton->setMaximumWidth(250);
  connect(m_openLastRomButton, SIGNAL(clicked()), this, SLOT(openLastRom()));

  const auto openButtonLayout = new QHBoxLayout();
  openButtonLayout->addWidget(openRomButton);
  openButtonLayout->addWidget(m_openLastRomButton);
  openButtonLayout->setAlignment(Qt::AlignHCenter);

  // level select button
  m_levelSelectButton = new QPushButton(tr("Level Select..."));
  m_levelSelectButton->setMaximumWidth(250);
  m_levelSelectButton->setDisabled(true);
  connect(m_levelSelectButton, SIGNAL(clicked()), this, SLOT(showLevelSelectDialog()));

  const auto buttonLayout = new QVBoxLayout();
  buttonLayout->addLayout(openButtonLayout);
  buttonLayout->addWidget(m_levelSelectButton);
  buttonLayout->setAlignment(Qt::AlignHCenter);

  const auto buttonsWidget = new QWidget();
  buttonsWidget->setLayout(buttonLayout);
  setCentralWidget(buttonsWidget);

  updateRecentRomActions();
}

bool Window::openRom(const QString &path)
{
  m_rom.reset();
  m_game.reset();

  m_rom = make_shared<Rom>();
  if (!m_rom->open(path.toStdString())) {
    showError(tr("ROM Error"), tr("Failed to open ROM file"));
    m_rom.reset();
    return false;
  }

  m_game = GameFactory::build(m_rom);
  if (!m_game) {
    showError(tr("ROM Error"), tr("Failed to identify game"));
    m_rom.reset();
    return false;
  }

  LOG() << "ROM identified";
  LOG() << "Domestic name: '" << m_rom->readDomesticName() << "'";

  addRecentRom(QFileInfo(path).absoluteFilePath());

  m_levelSelectAction->setEnabled(true);
  m_levelSelectButton->setEnabled(true);
  m_saveRomAction->setEnabled(false);
  m_relocateLevelsAction->setEnabled(m_game->canRelocateLevels());
  m_romInfoAction->setEnabled(false);

  if (m_romInfo) {
    delete m_romInfo;
    m_romInfo = nullptr;
  }

  return true;
}

bool Window::openProject(const QString& path)
{
  m_rom.reset();
  m_game.reset();

  try {
    m_game = GameFactory::buildDisassembly(path.toStdString());
  } catch (const exception& e) {
    showError(tr("Project Error"), tr("Failed to open project: ") + e.what());
    return false;
  }

  if (!m_game) {
    showError(tr("Project Error"),
        tr("Failed to identify a supported Sonic 2 or Sonic 3 disassembly project."));
    return false;
  }

  LOG() << "Project identified: " << m_game->getIdentifier();

  m_levelSelectAction->setEnabled(true);
  m_levelSelectButton->setEnabled(true);
  m_saveRomAction->setEnabled(false);
  m_relocateLevelsAction->setEnabled(false);
  m_romInfoAction->setEnabled(false);

  if (m_romInfo) {
    delete m_romInfo;
    m_romInfo = nullptr;
  }

  return true;
}

void Window::openLevel(const QString& level)
{
  bool parsed = false;
  const unsigned int levelIdx = level.toUInt(&parsed);
  if (parsed) {
    levelSelected(levelIdx);
  } else {
    showError(tr("Level Error"), tr("Failed to parse level index"));
  }
}

void Window::exportBinary(const QString& fileName)
{
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly)) {
    showError(tr("Export Binary"), tr("Failed to open file"));
    return;
  }

  auto& map = m_level->getMap();

  for (auto y = 0; y < map.getHeight(); y++) {
    for (auto x = 0; x < map.getWidth(); x++) {
      for (auto l = 0; l < 2; l++) {
        auto value = map.getValue(l, x, y);
        const char byte = static_cast<char>(value);
        file.write(&byte, 1);
      }
    }
  }

  showInfo(tr("Export Binary"), tr("Map exported successfully."));
}

void Window::exportPng(const QString& fileName)
{
  QImage image(m_mapEditor->getWidth(), m_mapEditor->getHeight(), QImage::Format_ARGB32);
  m_mapEditor->drawToImage(image);
  if (image.save(fileName, "PNG")) {
    showInfo(tr("Export PNG"), tr("Map exported successfully."));
  } else {
    showError(tr("Export ROM"), tr("Failed to save map to PNG."));
  }
}

void Window::showOpenRomDialog()
{
  QFileDialog dialog(this, tr("Open ROM"), QString(), tr("ROM Files (*.bin)"));
  dialog.setFileMode(QFileDialog::ExistingFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && !defined(Q_OS_WIN)
  dialog.setOption(QFileDialog::DontUseNativeDialog, true);
#endif
  if (!dialog.exec()) {
    return;
  }
  const QString fileName = dialog.selectedFiles().value(0);
  if (!fileName.isEmpty()) {
    openRomFromUserAction(fileName);
  }
}

void Window::showOpenProjectDialog()
{
  QFileDialog dialog(this, tr("Open Project"), QString(),
      tr("Project Files (*.ini);;All Files (*)"));
  dialog.setFileMode(QFileDialog::ExistingFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && !defined(Q_OS_WIN)
  dialog.setOption(QFileDialog::DontUseNativeDialog, true);
#endif
  if (!dialog.exec()) {
    return;
  }
  const QString fileName = dialog.selectedFiles().value(0);
  if (!fileName.isEmpty()) {
    openProjectFromUserAction(fileName);
  }
}

void Window::openLastRom()
{
  const QString fileName = recentRoms().value(0);
  if (!fileName.isEmpty()) {
    openRomFromUserAction(fileName);
  }
}

void Window::openRecentRom()
{
  const auto action = qobject_cast<QAction*>(sender());
  if (!action) {
    return;
  }

  const QString fileName = action->data().toString();
  if (!fileName.isEmpty()) {
    openRomFromUserAction(fileName);
  }
}

void Window::showLevelSelectDialog()
{
  if (m_level) {
    const QMessageBox::StandardButton reply = QMessageBox::question(this,
          tr("Close Level"),
          tr("Are you sure you want to close the current level?"),
          QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
      return;
    }
  }

  if (m_levelSelect) {
    delete m_levelSelect;
    m_levelSelect = nullptr;
  }

  m_levelSelect = new LevelSelect(this, m_game);
  connect(m_levelSelect, SIGNAL(levelSelected(int)), this, SLOT(levelSelected(int)));

  m_levelSelect->show();
}

void Window::saveRom()
{
  trySaveRom();
}

void Window::showExportBinaryDialog()
{
  if (!m_level) {
    return;
  }

  QFileDialog dialog(this, tr("Export Binary"), QString(), tr("Binary Files (*.bin)"));
  dialog.setAcceptMode(QFileDialog::AcceptSave);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && !defined(Q_OS_WIN)
  dialog.setOption(QFileDialog::DontUseNativeDialog, true);
#endif
  if (!dialog.exec()) {
    return;
  }
  const auto fileName = dialog.selectedFiles().value(0);
  if (fileName.isEmpty()) {
    return;
  }

  exportBinary(fileName);
}

void Window::showExportPngDialog()
{
  if (!m_level) {
    return;
  }

  QFileDialog dialog(this, tr("Export PNG"), QString(), tr("PNG Files (*.png)"));
  dialog.setAcceptMode(QFileDialog::AcceptSave);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && !defined(Q_OS_WIN)
  dialog.setOption(QFileDialog::DontUseNativeDialog, true);
#endif
  if (!dialog.exec()) {
    return;
  }
  const auto fileName = dialog.selectedFiles().value(0);
  if (fileName.isEmpty()) {
    return;
  }

  exportPng(fileName);
}

void Window::undo()
{
  m_mapEditor->undo();
}

void Window::redo()
{
  m_mapEditor->redo();
}

void Window::showPatternEditor()
{
  if (!m_level) {
    return;
  }

  PatternEditor editor(this, m_level);
  connect(&editor, SIGNAL(patternModified()), this, SLOT(patternModified()));
  editor.exec();
}

void Window::showPaletteEditor()
{
  if (!m_level) {
    return;
  }

  PaletteEditor editor(this, m_level);
  connect(&editor, SIGNAL(paletteModified()), this, SLOT(paletteModified()));
  editor.exec();
}

void Window::showBlockEditor()
{
  if (!m_level) {
    return;
  }

  auto* editor = new BlockEditor(this, m_level);
  editor->setAttribute(Qt::WA_DeleteOnClose);
  connect(editor, SIGNAL(blocksModified()), this, SLOT(blocksModified()));
  editor->show();
}

void Window::showChunkEditor()
{
  if (!m_level) {
    return;
  }

  const auto selectedChunk = m_mapEditor ? m_mapEditor->getSelectedChunk() : 0;
  auto* editor = new ChunkEditor(this, m_level, selectedChunk);
  editor->setAttribute(Qt::WA_DeleteOnClose);
  connect(editor, SIGNAL(chunksModified()), this, SLOT(chunksModified()));
  editor->show();
}

void Window::actualSize()
{
  if (!m_mapEditor) {
    return;
  }

  m_mapEditor->actualSize();
}

void Window::zoomIn()
{
  if (!m_mapEditor) {
    return;
  }

  m_mapEditor->zoomIn();
}

void Window::zoomOut()
{
  if (!m_mapEditor) {
    return;
  }

  m_mapEditor->zoomOut();
}

void Window::showPaletteInspector()
{
  if (!m_paletteInspector) {
    m_paletteInspector = new PaletteInspector(this, m_level);
  }

  m_paletteInspector->show();
}

void Window::showPatternInspector()
{
  if (!m_patternInspector) {
    m_patternInspector = new PatternInspector(this, m_level);
  }

  m_patternInspector->show();
}

void Window::showBlockInspector()
{
  if (!m_blockInspector) {
    m_blockInspector = new BlockInspector(this, m_level);
  }

  m_blockInspector->show();
}

void Window::showChunkInspector()
{
  if (!m_chunkInspector) {
    m_chunkInspector = new ChunkInspector(this, m_level);
  }

  m_chunkInspector->show();
}

void Window::showRomInfo()
{
  if (!m_rom || !m_game) {
    return;
  }

  if (!m_romInfo) {
    m_romInfo = new RomInfo(this, *m_rom, *m_game);
  }

  m_romInfo->show();
}

void Window::relocateLevels()
{
  try {
    const QMessageBox::StandardButton confirmation = QMessageBox::question(this,
          tr("Relocate Levels"),
          tr("Relocating levels will permanently modify the structure of this ROM. ") +
                tr("The current level will be closed and any unsaved changes will be lost.\n\n") +
                tr("Are you sure you want to continue?"),
          QMessageBox::Yes | QMessageBox::No);
    if (confirmation == QMessageBox::No) {
      return;
    }

    delete m_mapEditor;
    m_mapEditor = nullptr;
    m_level.reset();

    if (m_game->relocateLevels(false)) {
      showInfo(tr("Relocate Levels"), tr("Levels relocated successfully."));
      return;
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(this,
          tr("Relocate Levels"),
          tr("Levels could not be relocated safely. Would you like to try anyway?"),
          QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
      return;
    }

    if (m_game->relocateLevels(true)) {
      showInfo(tr("Relocate Levels"), tr("Levels relocated successfully (unsafe)."));
    } else {
      showError(tr("Relocate Levels"), tr("Level relocation was attempted but rolled back."));
    }
  } catch (exception& e) {
    showError(tr("Relocate Levels"), tr("Exception while relocating levels: ") + e.what());
  }
}

void Window::levelSelected(int levelIdx)
{
  closeCurrentLevel();

  if (!m_game) {
    showError(tr("Level Error"), tr("Cannot load level until a ROM or project has been loaded"));
    return;
  }

  LOG() << "Loading level: " << levelIdx << " (" << m_game->getTitleCards()[levelIdx] << ")";

  m_level = m_game->loadLevel(levelIdx);
  if (!m_level) {
    showError(tr("Level Error"), tr("Failed to load level"));
    return;
  }

  m_levelIdx = levelIdx;

  if (m_game->canSave()) {
    m_saveRomAction->setEnabled(true);
  }

  m_exportBinaryAction->setEnabled(true);
  m_exportPngAction->setEnabled(true);
  m_paletteEditorAction->setEnabled(true);
  m_patternEditorAction->setEnabled(true);
  m_blockEditorAction->setEnabled(true);
  m_chunkEditorAction->setEnabled(true);

  m_inspectorsMenu->setEnabled(true);
  m_romInfoAction->setEnabled(m_rom != nullptr);

  m_mapEditor = new MapEditor(this, m_level);
  connect(m_mapEditor, SIGNAL(currentTile(uint16_t,uint16_t,uint8_t)), this, SLOT(currentTile(uint16_t,uint16_t,uint8_t)));
  connect(m_mapEditor, SIGNAL(noTile()), this, SLOT(noTile()));
  connect(m_mapEditor, SIGNAL(undosRedosChanged(size_t,size_t)), this, SLOT(undosRedosChanged(size_t,size_t)));
  connect(m_mapEditor, SIGNAL(mapModified()), this, SLOT(mapModified()));
  this->setCentralWidget(m_mapEditor);

  m_hasUnsavedChanges = false;
}

void Window::currentTile(uint16_t x, uint16_t y, uint8_t value)
{
  m_statusBar->showMessage(
      QString("[%1, %2]: 0x%3")
          .arg(x)
          .arg(y)
          .arg(QString("%1")
               .arg(value, 1, 16)
               .toUpper()
               .rightJustified(2, '0')));
}

void Window::noTile()
{
  m_statusBar->showMessage(QString());
}

void Window::undosRedosChanged(size_t undos, size_t redos)
{
  LOG() << "Undos: " << undos << ", redos: " << redos;

  m_undoAction->setEnabled(undos > 0);
  m_redoAction->setEnabled(redos > 0);
}

void Window::mapModified()
{
  m_hasUnsavedChanges = true;
}

void Window::paletteModified()
{
  if (m_patternInspector) {
    m_patternInspector->refresh();
  }
  if (m_blockInspector) {
    m_blockInspector->refresh();
  }
  if (m_chunkInspector) {
    m_chunkInspector->refresh();
  }
  if (m_mapEditor) {
    m_mapEditor->refreshChunks();
  }
  m_hasUnsavedChanges = true;
}

void Window::patternModified()
{
  if (m_patternInspector) {
    m_patternInspector->refresh();
  }
  if (m_blockInspector) {
    m_blockInspector->refresh();
  }
  if (m_chunkInspector) {
    m_chunkInspector->refresh();
  }
  if (m_mapEditor) {
    m_mapEditor->refreshChunks();
  }
  m_hasUnsavedChanges = true;
}

void Window::blocksModified()
{
  if (m_blockInspector) {
    m_blockInspector->refresh();
  }
  if (m_chunkInspector) {
    m_chunkInspector->refresh();
  }
  if (m_mapEditor) {
    m_mapEditor->refreshChunks();
  }
  m_hasUnsavedChanges = true;
}

void Window::chunksModified()
{
  if (m_chunkInspector) {
    m_chunkInspector->refresh();
  }
  if (m_mapEditor) {
    m_mapEditor->refreshChunks();
  }
  m_hasUnsavedChanges = true;
}

bool Window::confirmCloseCurrentLevel()
{
  if (!m_level) {
    return true;
  }

  const QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Close Level"),
        tr("Are you sure you want to close the current level?"),
        QMessageBox::Yes | QMessageBox::No);
  return reply != QMessageBox::No;
}

void Window::closeCurrentLevel()
{
  m_inspectorsMenu->setEnabled(false);

  delete m_paletteInspector;
  m_paletteInspector = nullptr;

  delete m_patternInspector;
  m_patternInspector = nullptr;

  delete m_blockInspector;
  m_blockInspector = nullptr;

  delete m_chunkInspector;
  m_chunkInspector = nullptr;

  delete m_mapEditor;
  m_mapEditor = nullptr;

  m_level.reset();
  m_hasUnsavedChanges = false;

  m_saveRomAction->setEnabled(false);
  m_exportBinaryAction->setEnabled(false);
  m_exportPngAction->setEnabled(false);
  m_paletteEditorAction->setEnabled(false);
  m_patternEditorAction->setEnabled(false);
  m_blockEditorAction->setEnabled(false);
  m_chunkEditorAction->setEnabled(false);
  m_actualSizeAction->setEnabled(false);
  m_zoomInAction->setEnabled(false);
  m_zoomOutAction->setEnabled(false);
  m_undoAction->setEnabled(false);
  m_redoAction->setEnabled(false);
}

bool Window::openRomFromUserAction(const QString& path)
{
  if (!confirmCloseCurrentLevel()) {
    return false;
  }

  closeCurrentLevel();

  if (!openRom(path)) {
    return false;
  }

  showLevelSelectDialog();
  return true;
}

bool Window::openProjectFromUserAction(const QString& path)
{
  if (!confirmCloseCurrentLevel()) {
    return false;
  }

  closeCurrentLevel();

  if (!openProject(path)) {
    return false;
  }

  showLevelSelectDialog();
  return true;
}

QStringList Window::recentRoms() const
{
  QSettings settings(SettingsOrganization, SettingsApplication);
  return settings.value(RecentRomsKey).toStringList();
}

void Window::setRecentRoms(const QStringList& paths)
{
  QSettings settings(SettingsOrganization, SettingsApplication);
  settings.setValue(RecentRomsKey, paths);
}

void Window::addRecentRom(const QString& path)
{
  if (path.isEmpty()) {
    return;
  }

  QStringList paths = recentRoms();
  paths.removeAll(path);
  paths.prepend(path);

  while (paths.size() > MaxRecentRoms) {
    paths.removeLast();
  }

  setRecentRoms(paths);
  updateRecentRomActions();
}

void Window::updateRecentRomActions()
{
  const QStringList paths = recentRoms();
  const bool hasRecentRoms = !paths.isEmpty();

  if (m_openLastRomButton) {
    m_openLastRomButton->setEnabled(hasRecentRoms);
  }
  if (!m_openRecentMenu) {
    return;
  }

  m_openRecentMenu->clear();
  m_openRecentMenu->setEnabled(hasRecentRoms);

  for (int i = 0; i < paths.size(); i++) {
    const QString& path = paths.at(i);
    QString text = QFileInfo(path).fileName();
    if (text.isEmpty()) {
      text = path;
    }
    text.replace("&", "&&");

    auto action = new QAction(tr("&%1 %2").arg(i + 1).arg(text), m_openRecentMenu);
    action->setData(path);
    action->setStatusTip(path);
    connect(action, SIGNAL(triggered()), this, SLOT(openRecentRom()));
    m_openRecentMenu->addAction(action);
  }
}

bool Window::trySaveRom()
{
  if (!m_game || !m_level) {
    return false;
  }

  try {
    if (m_game->save(m_levelIdx, *m_level)) {
      QMessageBox::information(this,
          tr("Save ROM"),
          tr("Level saved successfully."),
          QMessageBox::StandardButton::Ok);
      m_hasUnsavedChanges = false;
      return true;
    }

    QMessageBox::warning(this,
        tr("Save ROM"),
        tr("There was not enough space to save this level. You may need to relocate the levels in this ROM."),
        QMessageBox::StandardButton::Ok);
    return false;
  } catch (const exception& e) {
    // show other error occurred
    QMessageBox::warning(this,
        tr("Save ROM"),
        tr("Something went wrong while saving this level: ") + e.what(),
        QMessageBox::StandardButton::Ok);
    return false;
  }
}

void Window::closeEvent(QCloseEvent* event)
{
  if (!m_hasUnsavedChanges || !m_level) {
    event->accept();
    return;
  }

  if (!m_game || !m_game->canSave()) {
    const auto reply = QMessageBox::question(this,
        tr("Quit"),
        tr("You have unsaved changes that will be lost.\n\nAre you sure you want to quit?"),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      event->accept();
    } else {
      event->ignore();
    }
    return;
  }

  const auto reply = QMessageBox::warning(this,
      tr("Quit"),
      tr("You have unsaved changes.\n\nDo you want to save them before quitting?"),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
      QMessageBox::Save);

  switch (reply) {
  case QMessageBox::Save:
    if (trySaveRom()) {
      event->accept();
    } else {
      event->ignore();
    }
    break;
  case QMessageBox::Discard:
    event->accept();
    break;
  default:
    event->ignore();
    break;
  }
}

void Window::createFileMenu()
{
  // open rom
  m_openRomAction = new QAction(tr("&Open ROM..."));
  m_openRomAction->setShortcuts(QKeySequence::Open);
  connect(m_openRomAction, SIGNAL(triggered()), this, SLOT(showOpenRomDialog()));

  // open project
  m_openProjectAction = new QAction(tr("Open &Project..."));
  connect(m_openProjectAction, SIGNAL(triggered()), this, SLOT(showOpenProjectDialog()));

  // level select
  m_levelSelectAction = new QAction(tr("&Level Select..."));
  m_levelSelectAction->setDisabled(true);
  connect(m_levelSelectAction, SIGNAL(triggered()), this, SLOT(showLevelSelectDialog()));

  // save rom
  m_saveRomAction = new QAction(tr("&Save ROM"));
  m_saveRomAction->setDisabled(true);
  connect(m_saveRomAction, SIGNAL(triggered()), this, SLOT(saveRom()));

  // file menu
  auto fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(m_openRomAction);
  fileMenu->addAction(m_openProjectAction);
  m_openRecentMenu = fileMenu->addMenu(tr("Open &Recent"));
  updateRecentRomActions();
  fileMenu->addSeparator()->setSeparator(true);
  fileMenu->addAction(m_levelSelectAction);
  fileMenu->addSeparator();
  fileMenu->addAction(m_saveRomAction);
}

void Window::createEditMenu()
{
  auto editMenu = menuBar()->addMenu(tr("&Edit"));

  m_undoAction = new QAction(tr("Undo"));
  m_undoAction->setShortcuts(QKeySequence::Undo);
  m_undoAction->setDisabled(true);
  connect(m_undoAction, SIGNAL(triggered()), this, SLOT(undo()));

  m_redoAction = new QAction(tr("Redo"));
  m_redoAction->setShortcuts(QKeySequence::Redo);
  m_redoAction->setDisabled(true);
  connect(m_redoAction, SIGNAL(triggered()), this, SLOT(redo()));

  m_paletteEditorAction = new QAction(tr("Palette Editor..."));
  m_paletteEditorAction->setDisabled(true);
  connect(m_paletteEditorAction, SIGNAL(triggered()), this, SLOT(showPaletteEditor()));

  m_patternEditorAction = new QAction(tr("8x8 Pattern Editor..."));
  m_patternEditorAction->setDisabled(true);
  connect(m_patternEditorAction, SIGNAL(triggered()), this, SLOT(showPatternEditor()));

  m_blockEditorAction = new QAction(tr("16x16 Block Editor..."));
  m_blockEditorAction->setDisabled(true);
  connect(m_blockEditorAction, SIGNAL(triggered()), this, SLOT(showBlockEditor()));

  m_chunkEditorAction = new QAction(tr("128x128 Chunk Editor..."));
  m_chunkEditorAction->setDisabled(true);
  connect(m_chunkEditorAction, SIGNAL(triggered()), this, SLOT(showChunkEditor()));

  editMenu->addAction(m_undoAction);
  editMenu->addAction(m_redoAction);
  editMenu->addSeparator();
  editMenu->addAction(m_paletteEditorAction);
  editMenu->addAction(m_patternEditorAction);
  editMenu->addAction(m_blockEditorAction);
  editMenu->addAction(m_chunkEditorAction);
}

void Window::createViewMenu()
{
  auto viewMenu = menuBar()->addMenu(tr("&View"));

  // wire up inspectors
  auto inspectPalettesAction = new QAction(tr("Palettes"), this);
  connect(inspectPalettesAction, SIGNAL(triggered()), this, SLOT(showPaletteInspector()));
  auto inspectPatternsAction = new QAction(tr("Patterns (8x8)"), this);
  connect(inspectPatternsAction, SIGNAL(triggered()), this, SLOT(showPatternInspector()));
  auto inspectBlocksAction = new QAction(tr("Blocks (16x16)"), this);
  connect(inspectBlocksAction, SIGNAL(triggered()), this, SLOT(showBlockInspector()));
  auto inspectChunksAction = new QAction(tr("Chunks (128x128)"), this);
  connect(inspectChunksAction, SIGNAL(triggered()), this, SLOT(showChunkInspector()));

  // build inspectors sub-menu
  m_inspectorsMenu = viewMenu->addMenu(tr("&Inspectors"));
  m_inspectorsMenu->setDisabled(true);
  m_inspectorsMenu->addAction(inspectPalettesAction);
  m_inspectorsMenu->addSeparator();
  m_inspectorsMenu->addAction(inspectPatternsAction);
  m_inspectorsMenu->addAction(inspectBlocksAction);
  m_inspectorsMenu->addAction(inspectChunksAction);

  // zoom
  m_actualSizeAction = new QAction(tr("Actual Size"), this);
  m_actualSizeAction->setDisabled(true);
  connect(m_actualSizeAction, SIGNAL(triggered()), this, SLOT(actualSize()));
  m_zoomInAction = new QAction(tr("Zoom In"), this);
  m_zoomInAction->setDisabled(true);
  connect(m_zoomInAction, SIGNAL(triggered()), this, SLOT(zoomIn()));
  m_zoomOutAction = new QAction(tr("Zoom Out"), this);
  m_zoomOutAction->setDisabled(true);
  connect(m_zoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOut()));

  viewMenu->addSeparator();
  viewMenu->addAction(m_actualSizeAction);
  viewMenu->addAction(m_zoomInAction);
  viewMenu->addAction(m_zoomOutAction);
}

void Window::createToolsMenu()
{
  auto toolsMenu = menuBar()->addMenu(tr("&Tools"));

  m_romInfoAction = new QAction(tr("ROM Info..."), this);
  m_romInfoAction->setDisabled(true);
  connect(m_romInfoAction, SIGNAL(triggered()), this, SLOT(showRomInfo()));

  m_relocateLevelsAction = new QAction(tr("Relocate Levels"), this);
  m_relocateLevelsAction->setDisabled(true);
  connect(m_relocateLevelsAction, SIGNAL(triggered()), this, SLOT(relocateLevels()));

  toolsMenu->addAction(m_romInfoAction);
  toolsMenu->addSeparator();
  toolsMenu->addAction(m_relocateLevelsAction);
}

void Window::createMapMenu()
{
  auto mapMenu = menuBar()->addMenu(tr("&Map"));

  // export binary
  m_exportBinaryAction = new QAction(tr("Export &Binary..."));
  m_exportBinaryAction->setDisabled(true);
  connect(m_exportBinaryAction, SIGNAL(triggered()), this, SLOT(showExportBinaryDialog()));

  // export png
  m_exportPngAction = new QAction(tr("Export &PNG..."));
  m_exportPngAction->setDisabled(true);
  connect(m_exportPngAction, SIGNAL(triggered()), this, SLOT(showExportPngDialog()));

  mapMenu->addAction(m_exportBinaryAction);
  mapMenu->addAction(m_exportPngAction);
}

void Window::showError(const QString& title, const QString& text)
{
  QMessageBox msgBox;
  msgBox.setWindowTitle(title);
  msgBox.setText(text);
  msgBox.exec();
}

void Window::showInfo(const QString& title, const QString& text)
{
  QMessageBox msgBox;
  msgBox.setWindowTitle(title);
  msgBox.setText(text);
  msgBox.exec();
}
