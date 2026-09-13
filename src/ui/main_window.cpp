#include "main_window.h"
#include "../theme/theme.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QFileInfo>
#include <QApplication>
#include <QDockWidget>
#include <QSettings>
#include <algorithm>
#include <QHash>

MainWindow::MainWindow() {
    setupUI();
    setupMenuBar();
    setupShortcuts();
    restoreLayout();
    updateWindowTitle();

    // Start with empty state
}

// An editor that forgets the layout you arranged on every launch does not feel
// finished. Geometry, panel sizes and panel visibility all persist.
void MainWindow::saveLayout() const {
    QSettings s;
    s.beginGroup(QStringLiteral("layout"));
    s.setValue(QStringLiteral("geometry"), saveGeometry());
    s.setValue(QStringLiteral("windowState"), saveState());
    s.setValue(QStringLiteral("horzSplitter"), horzSplitter_->saveState());
    s.setValue(QStringLiteral("vertSplitter"), vertSplitter_->saveState());
    s.setValue(QStringLiteral("judgeVisible"), cphDock_->isVisible());
    s.setValue(QStringLiteral("terminalVisible"), terminal_->isVisible());
    s.setValue(QStringLiteral("sidebarVisible"), fileExplorer_->isVisible());
    s.setValue(QStringLiteral("lastFolder"), fileExplorer_->rootPath());
    s.endGroup();
}

void MainWindow::restoreLayout() {
    QSettings s;
    s.beginGroup(QStringLiteral("layout"));

    const QByteArray geometry = s.value(QStringLiteral("geometry")).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        // First run keeps the existing behaviour of opening maximized. Set the
        // state rather than calling showMaximized(), so a restored geometry is
        // not overridden on every later launch.
        setWindowState(windowState() | Qt::WindowMaximized);
    }
    const QByteArray state = s.value(QStringLiteral("windowState")).toByteArray();
    if (!state.isEmpty()) restoreState(state);

    const QByteArray horz = s.value(QStringLiteral("horzSplitter")).toByteArray();
    if (!horz.isEmpty()) horzSplitter_->restoreState(horz);
    const QByteArray vert = s.value(QStringLiteral("vertSplitter")).toByteArray();
    if (!vert.isEmpty()) vertSplitter_->restoreState(vert);

    terminal_->setVisible(s.value(QStringLiteral("terminalVisible"), true).toBool());
    fileExplorer_->setVisible(s.value(QStringLiteral("sidebarVisible"), true).toBool());
    cphDock_->setVisible(s.value(QStringLiteral("judgeVisible"), false).toBool());

    // Reopening where you left off is most of what "resume work" means for a
    // contest folder. Only if it still exists.
    const QString folder = s.value(QStringLiteral("lastFolder")).toString();
    if (!folder.isEmpty() && QFileInfo(folder).isDir()) {
        fileExplorer_->setRootPath(folder);
    }
    s.endGroup();
}

void MainWindow::setupUI() {
    setWindowTitle("Valence");
    resize(1280, 800);
    setMinimumSize(800, 500);

    setStyleSheet(QString(
        "QMainWindow { background: %1; }"
        "QMenuBar {"
        "  background: %2;"
        "  color: %3;"
        "  border-bottom: 1px solid %4;"
        "  font-size: 13px;"
        "  padding: 3px 0;"
        "}"
        "QMenuBar::item {"
        "  padding: 5px 11px;"
        "  background: transparent;"
        "  border-radius: 4px;"
        "  margin: 1px 2px;"
        "}"
        "QMenuBar::item:selected {"
        "  background: rgba(255, 255, 255, 0.06);"
        "}"
        "QMenu {"
        "  background: %2;"
        "  color: %3;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "  font-size: 13px;"
        "  padding: 6px 0;"
        "}"
        "QMenu::item {"
        "  padding: 6px 28px 6px 14px;"
        "  border-radius: 4px;"
        "  margin: 1px 6px;"
        "}"
        "QMenu::item:selected {"
        "  background: rgba(255, 255, 255, 0.06);"
        "}"
        "QMenu::separator {"
        "  height: 1px;"
        "  background: %4;"
        "  margin: 4px 12px;"
        "}"
        "QSplitter::handle {"
        "  background: %4;"
        "  width: 1px;"
        "  height: 1px;"
        "}"
        // Tooltip styling
        "QToolTip {"
        "  background: %2;"
        "  color: %3;"
        "  border: 1px solid %4;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  font-size: 12px;"
        "}"
    ).arg(Theme::EditorBg.name(),
          Theme::TitlebarBg.name(),
          Theme::TextSecondary.name(QColor::HexArgb),
          Theme::Border.name(QColor::HexArgb)));

    // ── Layout ──
    // Sidebar | (Editor Tabs / Terminal)

    fileExplorer_ = new FileExplorer(this);
    fileExplorer_->setMinimumWidth(180);
    fileExplorer_->setMaximumWidth(400);

    tabWidget_ = new TabWidget(this);
    terminal_ = new TerminalWidget(this);
    statusBar_ = new StatusBar(this);

    // Vertical splitter: editor tabs on top, terminal on bottom
    vertSplitter_ = new QSplitter(Qt::Vertical);
    vertSplitter_->setChildrenCollapsible(true);
    vertSplitter_->addWidget(tabWidget_);
    vertSplitter_->addWidget(terminal_);
    vertSplitter_->setSizes({500, lastTerminalHeight_});
    vertSplitter_->setHandleWidth(1);

    // Horizontal splitter: sidebar on left, editor area on right
    horzSplitter_ = new QSplitter(Qt::Horizontal);
    horzSplitter_->setChildrenCollapsible(true);
    horzSplitter_->addWidget(fileExplorer_);
    horzSplitter_->addWidget(vertSplitter_);
    horzSplitter_->setSizes({lastSidebarWidth_, 1060});
    horzSplitter_->setHandleWidth(1);

    // Central widget
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(horzSplitter_, 1);
    mainLayout->addWidget(statusBar_);

    setCentralWidget(centralWidget);

    // ── Judge panel (CPH) — a dock so the user can resize, float or hide it ──
    cphPanel_ = new CphPanel(this);
    cphDock_ = new QDockWidget(this);
    cphDock_->setObjectName("JudgeDock");
    cphDock_->setWidget(cphPanel_);
    cphDock_->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    // No title bar: the panel paints its own header, and Qt's default one is
    // unstyled chrome that would break the theme.
    cphDock_->setTitleBarWidget(new QWidget(cphDock_));
    cphDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    cphDock_->setStyleSheet(QString("QDockWidget { border: none; background: %1; }")
                                .arg(Theme::PanelBg.name()));
    addDockWidget(Qt::RightDockWidgetArea, cphDock_);
    resizeDocks({cphDock_}, {360}, Qt::Horizontal);
    cphDock_->hide();   // opt-in: Ctrl+J, or the Run menu

    // Connections
    connect(tabWidget_, &TabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(tabWidget_, &TabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(fileExplorer_, &FileExplorer::fileDoubleClicked, this, &MainWindow::onFileDoubleClicked);
    connect(fileExplorer_, &FileExplorer::fileCreatedWithBoilerplate, this, &MainWindow::onFileCreatedWithBoilerplate);
    connect(fileExplorer_, &FileExplorer::openFolderRequested, this, &MainWindow::openFolder);

    // A file deleted or renamed on disk must not leave a tab pointing at a path
    // that no longer exists — saving such a tab would silently recreate the file.
    connect(fileExplorer_, &FileExplorer::fileDeleted, this, &MainWindow::onFileDeleted);
    connect(fileExplorer_, &FileExplorer::fileRenamed, this, &MainWindow::onFileRenamed);

    // The panel asks us to flush the editor to disk before it compiles. This is
    // a direct connection, so the save completes before runAll() reads the file.
    connect(cphPanel_, &CphPanel::saveBeforeRunRequested, this, &MainWindow::saveFile);
    connect(cphPanel_, &CphPanel::statusMessage, statusBar_, &StatusBar::setMessage);
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("New File", QKeySequence("Ctrl+N"), this, &MainWindow::newFile);
    fileMenu->addAction("Open File...", QKeySequence("Ctrl+O"), this, &MainWindow::openFile);
    fileMenu->addAction("Open Folder...", QKeySequence("Ctrl+Shift+O"), this, &MainWindow::openFolder);
    fileMenu->addSeparator();
    // NOTE: Ctrl+S is handled by EditorWidget, which emits saveRequested()
    auto* saveAction = fileMenu->addAction("Save", this, &MainWindow::saveFile);
    saveAction->setShortcut(QKeySequence()); // No shortcut — editor handles Ctrl+S
    fileMenu->addAction("Save As...", QKeySequence("Ctrl+Shift+S"), this, &MainWindow::saveFileAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", QKeySequence("Alt+F4"), this, &MainWindow::close);

    // Edit menu — NO keyboard shortcuts here!
    // All Ctrl+Z/Y/C/X/V/A are handled directly by EditorWidget::keyPressEvent
    // Menu items only work via mouse click
    auto* editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction("Undo", [this]() {
        if (auto* e = tabWidget_->currentEditor()) e->performUndo();
    });
    editMenu->addAction("Redo", [this]() {
        if (auto* e = tabWidget_->currentEditor()) e->performRedo();
    });
    editMenu->addSeparator();
    editMenu->addAction("Cut", [this]() {
        if (auto* e = tabWidget_->currentEditor()) e->cut();
    });
    editMenu->addAction("Copy", [this]() {
        if (auto* e = tabWidget_->currentEditor()) e->copy();
    });
    editMenu->addAction("Paste", [this]() {
        if (auto* e = tabWidget_->currentEditor()) e->paste();
    });
    editMenu->addSeparator();
    editMenu->addAction("Select All", [this]() {
        if (auto* e = tabWidget_->currentEditor()) e->selectAll();
    });

    auto* viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction("Toggle Terminal", QKeySequence("Ctrl+`"), this, &MainWindow::toggleTerminal);
    viewMenu->addAction("Toggle Sidebar", QKeySequence("Ctrl+B"), this, &MainWindow::toggleSidebar);
    viewMenu->addAction("Toggle Judge Panel", QKeySequence("Ctrl+J"), this, &MainWindow::toggleJudgePanel);

    auto* runMenu = menuBar()->addMenu("Run");
    runMenu->addAction("Run Test Cases", QKeySequence("Ctrl+Shift+J"), this, [this]() {
        if (cphDock_->isHidden()) toggleJudgePanel();
        cphPanel_->runAll();
    });
    runMenu->addSeparator();
    runMenu->addAction("▶  Build & Run", QKeySequence("F5"), this, &MainWindow::runCurrentFile);
    runMenu->addAction("Build Only", QKeySequence("Ctrl+Shift+B"), this, &MainWindow::buildCurrentFile);
}

void MainWindow::setupShortcuts() {
    // Tab management
    new QShortcut(QKeySequence("Ctrl+W"), this, [this]() {
        if (tabWidget_->count() > 0)
            onTabCloseRequested(tabWidget_->currentIndex());
    });
    new QShortcut(QKeySequence("Ctrl+Tab"), this, [this]() {
        if (tabWidget_->count() > 1)
            tabWidget_->setCurrentIndex((tabWidget_->currentIndex() + 1) % tabWidget_->count());
    });
}

EditorWidget* MainWindow::createEditor() {
    auto* editor = new EditorWidget();
    return editor;
}

void MainWindow::connectEditor(EditorWidget* editor) {
    connect(editor, &EditorWidget::cursorPositionChanged,
            this, &MainWindow::onCursorPositionChanged);
    connect(editor, &EditorWidget::modifiedChanged,
            this, &MainWindow::onModifiedChanged);
    connect(editor, &EditorWidget::saveRequested,
            this, &MainWindow::onEditorSaveRequested);
}

// ── Slots ──

void MainWindow::newFile() {
    auto* editor = createEditor();
    tabWidget_->addEditor(editor, "untitled");
    connectEditor(editor);
    editor->setFocus();
}

void MainWindow::openFile() {
    QString path = QFileDialog::getOpenFileName(this, "Open File", QString(),
        "C++ Files (*.cpp *.h *.hpp *.cc *.cxx);;All Files (*)");
    if (!path.isEmpty()) openFilePath(path);
}

void MainWindow::openFolder() {
    QString path = QFileDialog::getExistingDirectory(this, "Open Folder");
    if (!path.isEmpty()) {
        fileExplorer_->setRootPath(path);
    }
}

void MainWindow::openFilePath(const QString& path) {
    // Check if already open
    int existing = tabWidget_->findByFilePath(path);
    if (existing >= 0) {
        tabWidget_->setCurrentIndex(existing);
        return;
    }

    auto* editor = createEditor();
    if (!editor->openFile(path)) {
        delete editor;
        QMessageBox::warning(this, "Error", "Could not open file: " + path);
        return;
    }

    tabWidget_->addEditor(editor, editor->fileName());
    connectEditor(editor);
    editor->setFocus();
    statusBar_->setFileName(editor->fileName());
    updateWindowTitle();
}

void MainWindow::saveFile() {
    auto* editor = tabWidget_->currentEditor();
    if (!editor) return;

    if (editor->filePath().isEmpty()) {
        saveFileAs();
        return;
    }

    editor->saveFile();
    tabWidget_->updateTabLabel(tabWidget_->currentIndex());
    updateWindowTitle();
}

void MainWindow::saveFileAs() {
    auto* editor = tabWidget_->currentEditor();
    if (!editor) return;

    QString defaultDir = fileExplorer_->rootPath();
    QString path = QFileDialog::getSaveFileName(this, "Save File As", defaultDir,
        "C++ Files (*.cpp *.h *.hpp *.cc *.cxx);;All Files (*)");
    if (path.isEmpty()) return;

    editor->saveFileAs(path);
    tabWidget_->updateTabLabel(tabWidget_->currentIndex());
    syncJudgeTarget();
    updateWindowTitle();
}

void MainWindow::onTabChanged(int index) {
    if (index < 0) {
        statusBar_->setFileName("");
        statusBar_->setCursorPosition(0, 0);
        updateWindowTitle();
        return;
    }

    auto* editor = tabWidget_->editorAt(index);
    if (editor) {
        statusBar_->setFileName(editor->fileName());
        statusBar_->setCursorPosition(editor->currentRow(), editor->currentCol());
        editor->setFocus();
    }
    syncJudgeTarget();
    updateWindowTitle();
}

void MainWindow::onTabCloseRequested(int index) {
    auto* editor = tabWidget_->editorAt(index);
    if (!editor) return;

    if (editor->isModified()) {
        auto result = QMessageBox::question(this, "Unsaved Changes",
            QString("Save changes to %1?").arg(editor->fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Save) {
            if (editor->filePath().isEmpty()) {
                QString defaultDir = fileExplorer_->rootPath();
                QString path = QFileDialog::getSaveFileName(this, "Save File", defaultDir,
                    "C++ Files (*.cpp *.h *.hpp *.cc *.cxx);;All Files (*)");
                if (path.isEmpty()) return;
                editor->saveFileAs(path);
            } else {
                editor->saveFile();
            }
        } else if (result == QMessageBox::Cancel) {
            return;
        }
    }

    tabWidget_->closeTab(index);
    updateWindowTitle();
}

void MainWindow::onFileDoubleClicked(const QString& path) {
    openFilePath(path);
}

void MainWindow::onFileCreatedWithBoilerplate(const QString& path) {
    auto* editor = tabWidget_->currentEditor();
    if (editor && editor->filePath() == path) {
        // Select lines 4 to 5 (0-indexed). The cursor will be at line 4, col 4.
        // Wait, line 6 is index 5. We want to select the comment lines.
        // Line 4: "    // I wrote the boilerplate for you."
        // Line 5: "    // No need to thank me, just use Valence."
        // Selection end is row 6 col 4.
        editor->setSelection(4, 4, 6, 4);
    }
}

void MainWindow::onFileDeleted(const QString& path) {
    const int idx = tabWidget_->findByFilePath(path);
    if (idx < 0) return;

    // The file is already gone, so there is nothing to offer to save. Drop the
    // tab without the usual unsaved-changes prompt, which would only offer to
    // write the file back.
    tabWidget_->closeTab(idx);
    syncJudgeTarget();
    updateWindowTitle();
}

void MainWindow::onFileRenamed(const QString& oldPath, const QString& newPath) {
    const int idx = tabWidget_->findByFilePath(oldPath);
    if (idx < 0) return;

    if (auto* editor = tabWidget_->editorAt(idx)) {
        editor->setFilePath(newPath);
        tabWidget_->updateTabLabel(idx);
    }
    syncJudgeTarget();
    updateWindowTitle();
}

void MainWindow::onCursorPositionChanged(int row, int col) {
    statusBar_->setCursorPosition(row, col);
}

void MainWindow::onModifiedChanged(bool) {
    int idx = tabWidget_->currentIndex();
    if (idx >= 0) tabWidget_->updateTabLabel(idx);
    updateWindowTitle();
}

void MainWindow::onEditorSaveRequested() {
    saveFile();
}

void MainWindow::toggleTerminal() {
    // Remember the height the user dragged to, so hiding and re-showing the
    // terminal does not silently reset it to the default.
    if (terminal_->isVisible()) {
        int h = vertSplitter_->sizes().value(1);
        if (h > 0) lastTerminalHeight_ = h;
        terminal_->hide();
    } else {
        terminal_->show();
        vertSplitter_->setSizes({std::max(1, vertSplitter_->height() - lastTerminalHeight_),
                                 lastTerminalHeight_});
    }
}

void MainWindow::toggleSidebar() {
    if (fileExplorer_->isVisible()) {
        int w = horzSplitter_->sizes().value(0);
        if (w > 0) lastSidebarWidth_ = w;
        fileExplorer_->hide();
    } else {
        fileExplorer_->show();
        horzSplitter_->setSizes({lastSidebarWidth_,
                                 std::max(1, horzSplitter_->width() - lastSidebarWidth_)});
    }
}

void MainWindow::toggleJudgePanel() {
    if (cphDock_->isVisible()) {
        cphDock_->hide();
    } else {
        cphDock_->show();
        syncJudgeTarget();
    }
}

void MainWindow::syncJudgeTarget() {
    auto* editor = tabWidget_->currentEditor();
    cphPanel_->setTargetFile(editor ? editor->filePath() : QString());

    // Reporting "C++" for a .txt file is a small lie the status bar was telling
    // on every tab.
    static const QHash<QString, QString> languages = {
        {"cpp", "C++"}, {"cc", "C++"}, {"cxx", "C++"}, {"c++", "C++"},
        {"h", "C++"}, {"hpp", "C++"}, {"hh", "C++"},
        {"c", "C"}, {"py", "Python"}, {"java", "Java"}, {"rs", "Rust"},
        {"js", "JavaScript"}, {"ts", "TypeScript"}, {"json", "JSON"},
        {"md", "Markdown"}, {"txt", "Text"}, {"in", "Text"}, {"out", "Text"},
    };
    QString lang = "Plain";
    if (editor && !editor->filePath().isEmpty()) {
        const QString ext = QFileInfo(editor->filePath()).suffix().toLower();
        lang = languages.value(ext, ext.isEmpty() ? QStringLiteral("Plain") : ext.toUpper());
    } else if (editor) {
        lang = "C++";        // an unsaved buffer is a new solution
    }
    statusBar_->setLanguage(lang);
}

// Returns false if the user cancelled — the caller must then abort whatever it
// was doing (closing a tab, quitting the app).
bool MainWindow::confirmDiscardChanges() {
    for (int i = tabWidget_->count() - 1; i >= 0; --i) {
        auto* editor = tabWidget_->editorAt(i);
        if (!editor || !editor->isModified()) continue;

        tabWidget_->setCurrentIndex(i);
        auto result = QMessageBox::question(this, "Unsaved Changes",
            QString("Save changes to %1?").arg(editor->fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Cancel) return false;
        if (result == QMessageBox::Save) {
            if (editor->filePath().isEmpty()) {
                QString path = QFileDialog::getSaveFileName(this, "Save File",
                    fileExplorer_->rootPath(),
                    "C++ Files (*.cpp *.h *.hpp *.cc *.cxx);;All Files (*)");
                if (path.isEmpty()) return false;   // cancelled the save => cancel the close
                if (!editor->saveFileAs(path)) return false;
            } else if (!editor->saveFile()) {
                QMessageBox::warning(this, "Save Failed",
                    QString("Could not write %1.").arg(editor->filePath()));
                return false;
            }
        }
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (!confirmDiscardChanges()) {
        e->ignore();
        return;
    }
    cphPanel_->persist();   // don't lose the user's test cases on exit
    saveLayout();
    e->accept();
}

void MainWindow::updateWindowTitle() {
    auto* editor = tabWidget_->currentEditor();
    if (editor && !editor->fileName().isEmpty()) {
        QString title = editor->fileName();
        if (editor->isModified()) title += " ●";
        title += " — Valence";
        setWindowTitle(title);
    } else {
        setWindowTitle("Valence");
    }
}

void MainWindow::buildCurrentFile() {
    auto* editor = tabWidget_->currentEditor();
    if (!editor) return;

    // Auto-save first
    if (editor->filePath().isEmpty()) {
        saveFileAs();
        if (editor->filePath().isEmpty()) return; // User cancelled
    } else if (editor->isModified()) {
        editor->saveFile();
        tabWidget_->updateTabLabel(tabWidget_->currentIndex());
    }

    QString filePath = editor->filePath();
    QFileInfo fi(filePath);
    QString dir = fi.absolutePath();
    QString baseName = fi.completeBaseName();
    QString fileName = fi.fileName();

    // Show terminal
    terminal_->setVisible(true);
    terminal_->clearOutput();

    // cd to directory and compile
    terminal_->runCommand(QString("cd \"%1\"").arg(dir));
    terminal_->runCommand(QString("g++ \"%1\" -o \"%2.exe\"").arg(fileName, baseName));
}

void MainWindow::runCurrentFile() {
    auto* editor = tabWidget_->currentEditor();
    if (!editor) return;

    // Auto-save first
    if (editor->filePath().isEmpty()) {
        saveFileAs();
        if (editor->filePath().isEmpty()) return;
    } else if (editor->isModified()) {
        editor->saveFile();
        tabWidget_->updateTabLabel(tabWidget_->currentIndex());
    }

    QString filePath = editor->filePath();
    QFileInfo fi(filePath);
    QString dir = fi.absolutePath();
    QString baseName = fi.completeBaseName();
    QString fileName = fi.fileName();

    // Show terminal
    terminal_->setVisible(true);
    terminal_->clearOutput();

    // cd → compile → run (chained with &&)
    terminal_->runCommand(QString("cd \"%1\"").arg(dir));
    terminal_->runCommand(QString("g++ \"%1\" -o \"%2.exe\" ; if ($?) { .\\%2.exe }").arg(fileName, baseName));
}
