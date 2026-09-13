#pragma once
#include <QWidget>
#include <QFont>
#include <QTimer>
#include <QRect>
#include <QVariantAnimation>
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
    // Follows an on-disk rename without touching the buffer or the
    // modified flag — the content did not change, only its name.
    void setFilePath(const QString& path);
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
    void setSelection(int startRow, int startCol, int endRow, int endCol);

signals:
    void modifiedChanged(bool modified);
    void cursorPositionChanged(int row, int col);
    void saveRequested();

protected:
    bool event(QEvent* e) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
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

    // Block comment state at the START of each line. Maintained incrementally:
    // a full rescan on every keystroke is O(file) and shows up as lag well
    // before a file gets big.
    std::vector<bool> blockCommentState_;
    void rebuildCommentState(int fromRow);
    void rebuildCommentStateFull();

    // Rendering
    QFont font_;
    int charWidth_;
    int charHeight_;
    int ascent_;
    int scrollY_;
    int scrollX_ = 0;
    int gutterWidth_;
    int gutterPadding_;
    // True when the font really is fixed-pitch, which lets a whole token be
    // drawn in one call instead of a character at a time.
    bool monospaceExact_ = false;

    // Cursor
    QTimer* blinkTimer_;
    bool cursorVisible_;

    // Smooth scrolling. scrollTarget_ is where the view is heading, so a second
    // wheel tick mid-flight retargets rather than restarting from where the
    // animation happens to be.
    QVariantAnimation* scrollAnim_ = nullptr;
    int scrollTarget_ = 0;

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
    void handleTab(bool shift);
    void handleChar(char ch);
    void movePage(int direction, bool shift);

    // ── Indentation & bracket intelligence ──
    static constexpr int INDENT_WIDTH = 4;
    static char closerFor(char open);
    static bool isCloser(char c);
    static bool isIdentLike(char c);

    int indentWidthOf(const std::string& line) const;
    bool onlyWhitespaceBefore(int row, int col) const;
    // Walks back to the '{' this '}' closes, skipping braces that live inside
    // strings and comments. Returns false if the block is unbalanced.
    bool findMatchingOpenBrace(Position closePos, Position& out) const;
    void reindentClosingBrace();
    void indentBlock(int firstRow, int lastRow, bool unindent);
    void adjustColAfterIndent(int row, int delta);

    // (clipboard & undo/redo are public — see above)

    // Coordinate conversion
    int rowFromY(int y) const;
    int colFromX(int x, int row) const;
    int xFromCol(int col) const;
    int yFromRow(int row) const;

    // Scroll
    int maxScrollY() const;
    int maxScrollX() const;
    void clampScroll();
    void animateScrollTo(int targetY);
    void jumpScrollTo(int y);              // instant; keeps the animation in sync
    QRect cursorRect() const;
    int codeLeft() const { return gutterWidth_; }

    // Paint helpers
    void paintGutter(QPainter& p, int startRow, int endRow);
    void paintCode(QPainter& p, int startRow, int endRow);
    void paintCursor(QPainter& p);
    void paintSelection(QPainter& p, int row, int y);
};
