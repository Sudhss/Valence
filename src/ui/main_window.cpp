#include "main_window.h"
#include "../theme/theme.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QKeyEvent>
#include <QApplication>

MainWindow::MainWindow() {
    setupUI();
    setupMenuBar();
    setupShortcuts();
    updateWindowTitle();

    // Start with one empty tab
    newFile();
}

void MainWindow::setupUI() {
    setWindowTitle("Valence");
    resize(1280, 800);
    setMinimumSize(800, 500);

    // Apply dark window style
    setStyleSheet(QString(
        "QMainWindow { background: %1; }"
        "QMenuBar {"
        "  background: %2;"
        "  color: %3;"
        "  border-bottom: 1px solid %4;"
        "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
        "  font-size: 12px;"
        "  padding: 2px 0;"
        "}"
        "QMenuBar::item {"
        "  padding: 4px 12px;"
        "  background: transparent;"
        "}"
        "QMenuBar::item:selected {"
        "  background: rgba(255, 255, 255, 0.08);"
        "}"
        "QMenu {"
        "  background: %2;"
        "  color: %3;"
        "  border: 1px solid %4;"
        "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
        "  font-size: 12px;"
        "  padding: 4px 0;"
        "}"
        "QMenu::item {"
        "  padding: 6px 28px;"
        "}"
        "QMenu::item:selected {"
        "  background: rgba(0, 255, 156, 0.1);"
        "}"
        "QMenu::separator {"
        "  height: 1px;"
        "  background: %4;"
        "  margin: 4px 8px;"
        "}"
        "QSplitter::handle {"
        "  background: %4;"
        "  width: 1px;"
        "  height: 1px;"
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
    auto* vertSplitter = new QSplitter(Qt::Vertical);
    vertSplitter->setChildrenCollapsible(true);
    vertSplitter->addWidget(tabWidget_);
    vertSplitter->addWidget(terminal_);
    vertSplitter->setSizes({500, 200});
    vertSplitter->setHandleWidth(1);

    // Horizontal splitter: sidebar on left, editor area on right
    auto* horzSplitter = new QSplitter(Qt::Horizontal);
    horzSplitter->setChildrenCollapsible(true);
    horzSplitter->addWidget(fileExplorer_);
    horzSplitter->addWidget(vertSplitter);
    horzSplitter->setSizes({220, 1060});
    horzSplitter->setHandleWidth(1);

    // Central widget
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(horzSplitter, 1);
    mainLayout->addWidget(statusBar_);

    setCentralWidget(centralWidget);

    // Connections
    connect(tabWidget_, &TabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(tabWidget_, &TabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(fileExplorer_, &FileExplorer::fileDoubleClicked, this, &MainWindow::onFileDoubleClicked);
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("New File", this, &MainWindow::newFile, QKeySequence("Ctrl+N"));
    fileMenu->addAction("Open File...", this, &MainWindow::openFile, QKeySequence("Ctrl+O"));
    fileMenu->addAction("Open Folder...", this, &MainWindow::openFolder, QKeySequence("Ctrl+Shift+O"));
    fileMenu->addSeparator();
    // NOTE: Ctrl+S is handled by EditorWidget, which emits saveRequested()
    auto* saveAction = fileMenu->addAction("Save", this, &MainWindow::saveFile);
    saveAction->setShortcut(QKeySequence()); // No shortcut — editor handles Ctrl+S
    fileMenu->addAction("Save As...", this, &MainWindow::saveFileAs, QKeySequence("Ctrl+Shift+S"));
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QApplication::quit, QKeySequence("Alt+F4"));

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
    viewMenu->addAction("Toggle Terminal", this, &MainWindow::toggleTerminal, QKeySequence("Ctrl+`"));
    viewMenu->addAction("Toggle Sidebar", this, &MainWindow::toggleSidebar, QKeySequence("Ctrl+B"));
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

void MainWindow::connectEditor(EditorWidget* editor, int tabIndex) {
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
    int idx = tabWidget_->addEditor(editor, "untitled");
    connectEditor(editor, idx);
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

    int idx = tabWidget_->addEditor(editor, editor->fileName());
    connectEditor(editor, idx);
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

    QString path = QFileDialog::getSaveFileName(this, "Save File As", QString(),
        "C++ Files (*.cpp *.h *.hpp *.cc *.cxx);;All Files (*)");
    if (path.isEmpty()) return;

    editor->saveFileAs(path);
    tabWidget_->updateTabLabel(tabWidget_->currentIndex());
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
                QString path = QFileDialog::getSaveFileName(this, "Save File");
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
    terminal_->setVisible(!terminal_->isVisible());
}

void MainWindow::toggleSidebar() {
    fileExplorer_->setVisible(!fileExplorer_->isVisible());
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
