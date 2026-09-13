#pragma once
#include <QMainWindow>
#include "tab_widget.h"
#include "file_explorer.h"
#include "terminal_widget.h"
#include "status_bar.h"
#include "cph_panel.h"

class QDockWidget;
class QSplitter;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void newFile();
    void openFile();
    void openFolder();
    void saveFile();
    void saveFileAs();
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onFileDoubleClicked(const QString& path);
    void onFileCreatedWithBoilerplate(const QString& path);
    void onFileDeleted(const QString& path);
    void onFileRenamed(const QString& oldPath, const QString& newPath);
    void onCursorPositionChanged(int row, int col);
    void onModifiedChanged(bool modified);
    void onEditorSaveRequested();
    void toggleTerminal();
    void toggleSidebar();
    void toggleJudgePanel();
    void runCurrentFile();
    void buildCurrentFile();

private:
    TabWidget* tabWidget_ = nullptr;
    FileExplorer* fileExplorer_ = nullptr;
    TerminalWidget* terminal_ = nullptr;
    StatusBar* statusBar_ = nullptr;
    CphPanel* cphPanel_ = nullptr;
    QDockWidget* cphDock_ = nullptr;
    QSplitter* vertSplitter_ = nullptr;
    QSplitter* horzSplitter_ = nullptr;

    // Remembered extents so toggling a panel off and on again restores the
    // size the user chose, instead of snapping back to the default.
    int lastSidebarWidth_ = 220;
    int lastTerminalHeight_ = 200;

    void setupUI();
    void setupMenuBar();
    void setupShortcuts();
    void updateWindowTitle();
    void openFilePath(const QString& path);
    EditorWidget* createEditor();
    void connectEditor(EditorWidget* editor);
    bool confirmDiscardChanges();
    void syncJudgeTarget();
};
