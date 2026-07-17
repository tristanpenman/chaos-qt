#include "Window.h"

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
#include "ProjectExplorer.h"
#include "RomInfo.h"
#include "ChunkEditor.h"

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
    , levelSelect_(nullptr)
    , paletteInspector_(nullptr)
    , patternInspector_(nullptr)
    , blockInspector_(nullptr)
    , chunkInspector_(nullptr)
    , projectExplorer_(nullptr)
    , romInfo_(nullptr)
    , mapEditor_(nullptr)
    , openRomAction_(nullptr)
    , openProjectAction_(nullptr)
    , levelSelectAction_(nullptr)
    , saveRomAction_(nullptr)
    , exportBinaryAction_(nullptr)
    , exportPngAction_(nullptr)
    , undoAction_(nullptr)
    , redoAction_(nullptr)
    , paletteEditorAction_(nullptr)
    , patternEditorAction_(nullptr)
    , blockEditorAction_(nullptr)
    , chunkEditorAction_(nullptr)
    , actualSizeAction_(nullptr)
    , zoomInAction_(nullptr)
    , zoomOutAction_(nullptr)
    , relocateLevelsAction_(nullptr)
    , romInfoAction_(nullptr)
    , projectExplorerAction_(nullptr)
    , openRecentMenu_(nullptr)
    , inspectorsMenu_(nullptr)
    , statusBar_(nullptr)
    , openLastRomButton_(nullptr)
    , rom_(nullptr)
    , game_(nullptr)
    , level_(nullptr)
    , levelIdx_(0)
    , hasUnsavedChanges_(false)
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
    statusBar_ = new QStatusBar(this);
    statusBar_->showMessage(tr("Ready"));
    setStatusBar(statusBar_);

    // open rom button
    const auto openRomButton = new QPushButton(tr("Open ROM..."));
    openRomButton->setMaximumWidth(250);
    connect(openRomButton, &QPushButton::clicked, this, &Window::showOpenRomDialog);

    openLastRomButton_ = new QPushButton(tr("Open Last..."));
    openLastRomButton_->setMaximumWidth(250);
    connect(openLastRomButton_, &QPushButton::clicked, this, &Window::openLastRom);

    const auto openButtonLayout = new QHBoxLayout();
    openButtonLayout->addWidget(openRomButton);
    openButtonLayout->addWidget(openLastRomButton_);
    openButtonLayout->setAlignment(Qt::AlignHCenter);

    // level select button
    levelSelectButton_ = new QPushButton(tr("Level Select..."));
    levelSelectButton_->setMaximumWidth(250);
    levelSelectButton_->setDisabled(true);
    connect(levelSelectButton_, &QPushButton::clicked, this, &Window::showLevelSelectDialog);

    const auto buttonLayout = new QVBoxLayout();
    buttonLayout->addLayout(openButtonLayout);
    buttonLayout->addWidget(levelSelectButton_);
    buttonLayout->setAlignment(Qt::AlignHCenter);

    const auto buttonsWidget = new QWidget();
    buttonsWidget->setLayout(buttonLayout);
    setCentralWidget(buttonsWidget);

    updateRecentRomActions();
}

bool Window::openRom(const QString& path)
{
    rom_.reset();
    game_.reset();

    rom_ = make_shared<Rom>();
    if (!rom_->open(path.toStdString())) {
        showError(tr("ROM Error"), tr("Failed to open ROM file"));
        rom_.reset();
        return false;
    }

    game_ = GameFactory::build(rom_);
    if (!game_) {
        showError(tr("ROM Error"), tr("Failed to identify game"));
        rom_.reset();
        return false;
    }

    LOG() << "ROM identified";
    LOG() << "Domestic name: '" << rom_->readDomesticName() << "'";

    addRecentRom(QFileInfo(path).absoluteFilePath());

    levelSelectAction_->setEnabled(true);
    levelSelectButton_->setEnabled(true);
    saveRomAction_->setEnabled(false);
    relocateLevelsAction_->setEnabled(game_->canRelocateLevels());
    romInfoAction_->setEnabled(false);
    projectExplorerAction_->setEnabled(false);
    projectExplorerAction_->setChecked(false);

    delete projectExplorer_;
    projectExplorer_ = nullptr;

    if (romInfo_) {
        delete romInfo_;
        romInfo_ = nullptr;
    }

    return true;
}

bool Window::openProject(const QString& path)
{
    rom_.reset();
    game_.reset();

    try {
        game_ = GameFactory::buildDisassembly(path.toStdString());
    } catch (const exception& e) {
        showError(tr("Project Error"), tr("Failed to open project: ") + e.what());
        return false;
    }

    if (!game_) {
        showError(tr("Project Error"),
                tr("Failed to identify a supported Sonic 2 or Sonic 3 disassembly project."));
        return false;
    }

    LOG() << "Project identified: " << game_->getIdentifier();

    levelSelectAction_->setEnabled(true);
    levelSelectButton_->setEnabled(true);
    saveRomAction_->setEnabled(false);
    relocateLevelsAction_->setEnabled(false);
    romInfoAction_->setEnabled(false);
    projectExplorerAction_->setEnabled(!game_->getProjectResources().empty());
    projectExplorerAction_->setChecked(false);

    delete projectExplorer_;
    projectExplorer_ = nullptr;

    if (romInfo_) {
        delete romInfo_;
        romInfo_ = nullptr;
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

    auto& map = level_->getMap();

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
    QImage image(mapEditor_->getWidth(), mapEditor_->getHeight(), QImage::Format_ARGB32);
    mapEditor_->drawToImage(image);
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
    QFileDialog dialog(this, tr("Open Project"));
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
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
    if (level_) {
        const QMessageBox::StandardButton reply = QMessageBox::question(this,
          tr("Close Level"),
          tr("Are you sure you want to close the current level?"),
          QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }

    if (levelSelect_) {
        delete levelSelect_;
        levelSelect_ = nullptr;
    }

    levelSelect_ = new LevelSelect(this, game_);
    connect(levelSelect_, &LevelSelect::levelSelected, this, &Window::levelSelected);

    levelSelect_->show();
}

void Window::saveRom()
{
    trySaveRom();
}

void Window::showExportBinaryDialog()
{
    if (!level_) {
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
    if (!level_) {
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
    mapEditor_->undo();
}

void Window::redo()
{
    mapEditor_->redo();
}

void Window::showPatternEditor()
{
    if (!level_) {
        return;
    }

    PatternEditor editor(this, level_);
    connect(&editor, &PatternEditor::patternModified, this, &Window::patternModified);
    editor.exec();
}

void Window::showPaletteEditor()
{
    if (!level_) {
        return;
    }

    PaletteEditor editor(this, level_);
    connect(&editor, &PaletteEditor::paletteModified, this, &Window::paletteModified);
    editor.exec();
}

void Window::showBlockEditor()
{
    if (!level_) {
        return;
    }

    auto* editor = new BlockEditor(this, level_);
    editor->setAttribute(Qt::WA_DeleteOnClose);
    connect(editor, &BlockEditor::blocksModified, this, &Window::blocksModified);
    editor->show();
}

void Window::showChunkEditor()
{
    if (!level_) {
        return;
    }

    const auto selectedChunk = mapEditor_ ? mapEditor_->getSelectedChunk() : 0;
    auto* editor = new ChunkEditor(this, level_, selectedChunk);
    editor->setAttribute(Qt::WA_DeleteOnClose);
    connect(editor, &ChunkEditor::chunksModified, this, &Window::chunksModified);
    editor->show();
}

void Window::actualSize()
{
    if (!mapEditor_) {
        return;
    }

    mapEditor_->actualSize();
}

void Window::zoomIn()
{
    if (!mapEditor_) {
        return;
    }

    mapEditor_->zoomIn();
}

void Window::zoomOut()
{
    if (!mapEditor_) {
        return;
    }

    mapEditor_->zoomOut();
}

void Window::showPaletteInspector()
{
    if (!paletteInspector_) {
        paletteInspector_ = new PaletteInspector(this, level_);
    }

    paletteInspector_->show();
}

void Window::showPatternInspector()
{
    if (!patternInspector_) {
        patternInspector_ = new PatternInspector(this, level_);
    }

    patternInspector_->show();
}

void Window::showBlockInspector()
{
    if (!blockInspector_) {
        blockInspector_ = new BlockInspector(this, level_);
    }

    blockInspector_->show();
}

void Window::showChunkInspector()
{
    if (!chunkInspector_) {
        chunkInspector_ = new ChunkInspector(this, level_);
    }

    chunkInspector_->show();
}

void Window::toggleProjectExplorer()
{
    if (!game_ || !projectExplorerAction_->isChecked()) {
        delete projectExplorer_;
        projectExplorer_ = nullptr;
        return;
    }

    if (!projectExplorer_) {
        projectExplorer_ = new ProjectExplorer(this, game_->getProjectResources());
        projectExplorer_->setAttribute(Qt::WA_DeleteOnClose);
        connect(projectExplorer_, &QObject::destroyed, this, [this]() {
            projectExplorer_ = nullptr;
            projectExplorerAction_->setChecked(false);
        });
    }

    projectExplorer_->show();
    projectExplorer_->raise();
    projectExplorer_->activateWindow();
}

void Window::showRomInfo()
{
    if (!rom_ || !game_) {
        return;
    }

    if (!romInfo_) {
        romInfo_ = new RomInfo(this, *rom_, *game_);
    }

    romInfo_->show();
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

        delete mapEditor_;
        mapEditor_ = nullptr;
        level_.reset();

        if (game_->relocateLevels(false)) {
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

        if (game_->relocateLevels(true)) {
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

    if (!game_) {
        showError(tr("Level Error"), tr("Cannot load level until a ROM or project has been loaded"));
        return;
    }

    LOG() << "Loading level: " << levelIdx << " (" << game_->getTitleCards()[levelIdx] << ")";

    level_ = game_->loadLevel(levelIdx);
    if (!level_) {
        showError(tr("Level Error"), tr("Failed to load level"));
        return;
    }

    levelIdx_ = levelIdx;

    if (game_->canSave()) {
        saveRomAction_->setEnabled(true);
    }

    exportBinaryAction_->setEnabled(true);
    exportPngAction_->setEnabled(true);
    paletteEditorAction_->setEnabled(true);
    patternEditorAction_->setEnabled(true);
    blockEditorAction_->setEnabled(true);
    chunkEditorAction_->setEnabled(true);

    inspectorsMenu_->setEnabled(true);
    romInfoAction_->setEnabled(rom_ != nullptr);

    mapEditor_ = new MapEditor(this, level_);
    connect(mapEditor_, &MapEditor::currentTile, this, &Window::currentTile);
    connect(mapEditor_, &MapEditor::noTile, this, &Window::noTile);
    connect(mapEditor_, &MapEditor::undosRedosChanged, this, &Window::undosRedosChanged);
    connect(mapEditor_, &MapEditor::mapModified, this, &Window::mapModified);
    this->setCentralWidget(mapEditor_);

    hasUnsavedChanges_ = false;
}

void Window::currentTile(uint16_t x, uint16_t y, uint8_t value)
{
    statusBar_->showMessage(
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
    statusBar_->showMessage(QString());
}

void Window::undosRedosChanged(size_t undos, size_t redos)
{
    LOG() << "Undos: " << undos << ", redos: " << redos;

    undoAction_->setEnabled(undos > 0);
    redoAction_->setEnabled(redos > 0);
}

void Window::mapModified()
{
    hasUnsavedChanges_ = true;
}

void Window::paletteModified()
{
    if (patternInspector_) {
        patternInspector_->refresh();
    }
    if (blockInspector_) {
        blockInspector_->refresh();
    }
    if (chunkInspector_) {
        chunkInspector_->refresh();
    }
    if (mapEditor_) {
        mapEditor_->refreshChunks();
    }
    hasUnsavedChanges_ = true;
}

void Window::patternModified()
{
    if (patternInspector_) {
        patternInspector_->refresh();
    }
    if (blockInspector_) {
        blockInspector_->refresh();
    }
    if (chunkInspector_) {
        chunkInspector_->refresh();
    }
    if (mapEditor_) {
        mapEditor_->refreshChunks();
    }
    hasUnsavedChanges_ = true;
}

void Window::blocksModified()
{
    if (blockInspector_) {
        blockInspector_->refresh();
    }
    if (chunkInspector_) {
        chunkInspector_->refresh();
    }
    if (mapEditor_) {
        mapEditor_->refreshChunks();
    }
    hasUnsavedChanges_ = true;
}

void Window::chunksModified()
{
    if (chunkInspector_) {
        chunkInspector_->refresh();
    }
    if (mapEditor_) {
        mapEditor_->refreshChunks();
    }
    hasUnsavedChanges_ = true;
}

bool Window::confirmCloseCurrentLevel()
{
    if (!level_) {
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
    inspectorsMenu_->setEnabled(false);

    delete paletteInspector_;
    paletteInspector_ = nullptr;

    delete patternInspector_;
    patternInspector_ = nullptr;

    delete blockInspector_;
    blockInspector_ = nullptr;

    delete chunkInspector_;
    chunkInspector_ = nullptr;

    delete projectExplorer_;
    projectExplorer_ = nullptr;
    projectExplorerAction_->setChecked(false);

    delete mapEditor_;
    mapEditor_ = nullptr;

    level_.reset();
    hasUnsavedChanges_ = false;

    saveRomAction_->setEnabled(false);
    exportBinaryAction_->setEnabled(false);
    exportPngAction_->setEnabled(false);
    paletteEditorAction_->setEnabled(false);
    patternEditorAction_->setEnabled(false);
    blockEditorAction_->setEnabled(false);
    chunkEditorAction_->setEnabled(false);
    actualSizeAction_->setEnabled(false);
    zoomInAction_->setEnabled(false);
    zoomOutAction_->setEnabled(false);
    undoAction_->setEnabled(false);
    redoAction_->setEnabled(false);
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

    if (openLastRomButton_) {
        openLastRomButton_->setEnabled(hasRecentRoms);
    }
    if (!openRecentMenu_) {
        return;
    }

    openRecentMenu_->clear();
    openRecentMenu_->setEnabled(hasRecentRoms);

    for (int i = 0; i < paths.size(); i++) {
        const QString& path = paths.at(i);
        QString text = QFileInfo(path).fileName();
        if (text.isEmpty()) {
            text = path;
        }
        text.replace("&", "&&");

        auto action = new QAction(tr("&%1 %2").arg(i + 1).arg(text), openRecentMenu_);
        action->setData(path);
        action->setStatusTip(path);
        connect(action, &QAction::triggered, this, &Window::openRecentRom);
        openRecentMenu_->addAction(action);
    }
}

bool Window::trySaveRom()
{
    if (!game_ || !level_) {
        return false;
    }

    try {
        if (game_->save(levelIdx_, *level_)) {
            QMessageBox::information(this,
          tr("Save ROM"),
          tr("Level saved successfully."),
          QMessageBox::StandardButton::Ok);
            hasUnsavedChanges_ = false;
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
    if (!hasUnsavedChanges_ || !level_) {
        event->accept();
        return;
    }

    if (!game_ || !game_->canSave()) {
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
    openRomAction_ = new QAction(tr("&Open ROM..."), this);
    openRomAction_->setShortcuts(QKeySequence::Open);
    connect(openRomAction_, &QAction::triggered, this, &Window::showOpenRomDialog);

    // open project
    openProjectAction_ = new QAction(tr("Open &Project..."), this);
    connect(openProjectAction_, &QAction::triggered, this, &Window::showOpenProjectDialog);

    // level select
    levelSelectAction_ = new QAction(tr("&Level Select..."), this);
    levelSelectAction_->setDisabled(true);
    connect(levelSelectAction_, &QAction::triggered, this, &Window::showLevelSelectDialog);

    // save rom
    saveRomAction_ = new QAction(tr("&Save ROM"), this);
    saveRomAction_->setDisabled(true);
    connect(saveRomAction_, &QAction::triggered, this, &Window::saveRom);

    // file menu
    auto fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openRomAction_);
    fileMenu->addAction(openProjectAction_);
    openRecentMenu_ = fileMenu->addMenu(tr("Open &Recent"));
    updateRecentRomActions();
    fileMenu->addSeparator()->setSeparator(true);
    fileMenu->addAction(levelSelectAction_);
    fileMenu->addSeparator();
    fileMenu->addAction(saveRomAction_);
}

void Window::createEditMenu()
{
    auto editMenu = menuBar()->addMenu(tr("&Edit"));

    undoAction_ = new QAction(tr("Undo"), this);
    undoAction_->setShortcuts(QKeySequence::Undo);
    undoAction_->setDisabled(true);
    connect(undoAction_, &QAction::triggered, this, &Window::undo);

    redoAction_ = new QAction(tr("Redo"), this);
    redoAction_->setShortcuts(QKeySequence::Redo);
    redoAction_->setDisabled(true);
    connect(redoAction_, &QAction::triggered, this, &Window::redo);

    paletteEditorAction_ = new QAction(tr("Palette Editor..."), this);
    paletteEditorAction_->setDisabled(true);
    connect(paletteEditorAction_, &QAction::triggered, this, &Window::showPaletteEditor);

    patternEditorAction_ = new QAction(tr("8x8 Pattern Editor..."), this);
    patternEditorAction_->setDisabled(true);
    connect(patternEditorAction_, &QAction::triggered, this, &Window::showPatternEditor);

    blockEditorAction_ = new QAction(tr("16x16 Block Editor..."), this);
    blockEditorAction_->setDisabled(true);
    connect(blockEditorAction_, &QAction::triggered, this, &Window::showBlockEditor);

    chunkEditorAction_ = new QAction(tr("128x128 Chunk Editor..."), this);
    chunkEditorAction_->setDisabled(true);
    connect(chunkEditorAction_, &QAction::triggered, this, &Window::showChunkEditor);

    editMenu->addAction(undoAction_);
    editMenu->addAction(redoAction_);
    editMenu->addSeparator();
    editMenu->addAction(paletteEditorAction_);
    editMenu->addAction(patternEditorAction_);
    editMenu->addAction(blockEditorAction_);
    editMenu->addAction(chunkEditorAction_);
}

void Window::createViewMenu()
{
    auto viewMenu = menuBar()->addMenu(tr("&View"));

    projectExplorerAction_ = new QAction(tr("Project Explorer"), this);
    projectExplorerAction_->setCheckable(true);
    projectExplorerAction_->setDisabled(true);
    connect(projectExplorerAction_, &QAction::triggered, this, &Window::toggleProjectExplorer);

    viewMenu->addAction(projectExplorerAction_);
    viewMenu->addSeparator();

    // wire up inspectors
    auto inspectPalettesAction = new QAction(tr("Palettes"), this);
    connect(inspectPalettesAction, &QAction::triggered, this, &Window::showPaletteInspector);
    auto inspectPatternsAction = new QAction(tr("Patterns (8x8)"), this);
    connect(inspectPatternsAction, &QAction::triggered, this, &Window::showPatternInspector);
    auto inspectBlocksAction = new QAction(tr("Blocks (16x16)"), this);
    connect(inspectBlocksAction, &QAction::triggered, this, &Window::showBlockInspector);
    auto inspectChunksAction = new QAction(tr("Chunks (128x128)"), this);
    connect(inspectChunksAction, &QAction::triggered, this, &Window::showChunkInspector);

    // build inspectors sub-menu
    inspectorsMenu_ = viewMenu->addMenu(tr("&Inspectors"));
    inspectorsMenu_->setDisabled(true);
    inspectorsMenu_->addAction(inspectPalettesAction);
    inspectorsMenu_->addSeparator();
    inspectorsMenu_->addAction(inspectPatternsAction);
    inspectorsMenu_->addAction(inspectBlocksAction);
    inspectorsMenu_->addAction(inspectChunksAction);

    // zoom
    actualSizeAction_ = new QAction(tr("Actual Size"), this);
    actualSizeAction_->setDisabled(true);
    connect(actualSizeAction_, &QAction::triggered, this, &Window::actualSize);
    zoomInAction_ = new QAction(tr("Zoom In"), this);
    zoomInAction_->setDisabled(true);
    connect(zoomInAction_, &QAction::triggered, this, &Window::zoomIn);
    zoomOutAction_ = new QAction(tr("Zoom Out"), this);
    zoomOutAction_->setDisabled(true);
    connect(zoomOutAction_, &QAction::triggered, this, &Window::zoomOut);

    viewMenu->addSeparator();
    viewMenu->addAction(actualSizeAction_);
    viewMenu->addAction(zoomInAction_);
    viewMenu->addAction(zoomOutAction_);
}

void Window::createToolsMenu()
{
    auto toolsMenu = menuBar()->addMenu(tr("&Tools"));

    romInfoAction_ = new QAction(tr("ROM Info..."), this);
    romInfoAction_->setDisabled(true);
    connect(romInfoAction_, &QAction::triggered, this, &Window::showRomInfo);

    relocateLevelsAction_ = new QAction(tr("Relocate Levels"), this);
    relocateLevelsAction_->setDisabled(true);
    connect(relocateLevelsAction_, &QAction::triggered, this, &Window::relocateLevels);

    toolsMenu->addAction(romInfoAction_);
    toolsMenu->addSeparator();
    toolsMenu->addAction(relocateLevelsAction_);
}

void Window::createMapMenu()
{
    auto mapMenu = menuBar()->addMenu(tr("&Map"));

    // export binary
    exportBinaryAction_ = new QAction(tr("Export &Binary..."), this);
    exportBinaryAction_->setDisabled(true);
    connect(exportBinaryAction_, &QAction::triggered, this, &Window::showExportBinaryDialog);

    // export png
    exportPngAction_ = new QAction(tr("Export &PNG..."), this);
    exportPngAction_->setDisabled(true);
    connect(exportPngAction_, &QAction::triggered, this, &Window::showExportPngDialog);

    mapMenu->addAction(exportBinaryAction_);
    mapMenu->addAction(exportPngAction_);
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
