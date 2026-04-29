#pragma once
#include <QMainWindow>
#include "tab_widget.h"
#include "file_explorer.h"
#include "terminal_widget.h"
#include "status_bar.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

private slots:
    void newFile();
    void openFile();
    void openFolder();
    void saveFile();
    void saveFileAs();
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onFileDoubleClicked(const QString& path);
    void onCursorPositionChanged(int row, int col);
    void onModifiedChanged(bool modified);
    void onEditorSaveRequested();
    void toggleTerminal();
    void toggleSidebar();
    void runCurrentFile();
    void buildCurrentFile();

private:
    TabWidget* tabWidget_;
    FileExplorer* fileExplorer_;
    TerminalWidget* terminal_;
    StatusBar* statusBar_;
    QWidget* sidebarContainer_;
    QWidget* terminalContainer_;

    void setupUI();
    void setupMenuBar();
    void setupShortcuts();
    void updateWindowTitle();
    void openFilePath(const QString& path);
    EditorWidget* createEditor();
    void connectEditor(EditorWidget* editor, int tabIndex);
};
