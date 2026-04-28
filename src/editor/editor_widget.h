#pragma once
#include <QWidget>
#include <QFont>
#include <QTimer>
#include "../core/text_buffer.h"
#include "../core/undo_manager.h"
#include "../core/selection.h"
#include "cpp_highlighter.h"

class EditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit EditorWidget(QWidget* parent = nullptr);

    bool openFile(const QString& path);
    bool saveFile();
    bool saveFileAs(const QString& path);

    QString filePath() const;
    bool isModified() const;
    QString fileName() const;

    int currentRow() const;
    int currentCol() const;
    int totalLines() const;

    // For external line number area queries
    int firstVisibleRow() const;
    int lastVisibleRow() const;
    int getGutterWidth() const;
    int getCharHeight() const;
    int getScrollY() const;
    int getCharWidth() const;

    // Public editing operations (called by menus)
    void performUndo();
    void performRedo();
    void copy();
    void cut();
    void paste();
    void selectAll();

signals:
    void modifiedChanged(bool modified);
    void cursorPositionChanged(int row, int col);
    void saveRequested();

protected:
    bool event(QEvent* e) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void toggleCursorBlink();

private:
    // Core data
    TextBuffer buffer_;
    UndoManager undoManager_;
    Position cursor_;
    Selection selection_;
    CppHighlighter highlighter_;

    // Block comment state per line
    std::vector<bool> blockCommentState_;
    void rebuildCommentState();

    // Rendering
    QFont font_;
    int charWidth_;
    int charHeight_;
    int ascent_;
    int scrollY_;
    int gutterWidth_;
    int gutterPadding_;

    // Cursor
    QTimer* blinkTimer_;
    bool cursorVisible_;

    // File
    QString filePath_;
    bool modified_;

    // Helpers
    void updateGutterWidth();
    void ensureCursorVisible();
    void setModified(bool m);
    void resetCursorBlink();

    // Cursor movement
    void moveCursorLeft(bool shift);
    void moveCursorRight(bool shift);
    void moveCursorUp(bool shift);
    void moveCursorDown(bool shift);
    void moveCursorHome(bool shift, bool ctrl);
    void moveCursorEnd(bool shift, bool ctrl);
    void moveCursorWordLeft(bool shift);
    void moveCursorWordRight(bool shift);

    // Selection helpers
    void updateSelectionForMove(bool shift);
    void deleteSelection();
    std::string getSelectedText() const;

    // Edit operations
    void handleBackspace(bool ctrl);
    void handleDelete();
    void handleEnter();
    void handleTab();
    void handleChar(char ch);

    // (clipboard & undo/redo are public — see above)

    // Coordinate conversion
    int rowFromY(int y) const;
    int colFromX(int x, int row) const;
    int xFromCol(int col) const;
    int yFromRow(int row) const;

    // Scroll
    int maxScrollY() const;
    void clampScroll();

    // Paint helpers
    void paintGutter(QPainter& p, int startRow, int endRow);
    void paintCode(QPainter& p, int startRow, int endRow);
    void paintCursor(QPainter& p);
    void paintSelection(QPainter& p, int row, int y);
};
