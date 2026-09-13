#include "editor_widget.h"
#include "../theme/theme.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QClipboard>
#include <QApplication>
#include <algorithm>
#include <cmath>

EditorWidget::EditorWidget(QWidget* parent) : QWidget(parent) {
    font_ = Theme::editorFont();
    QFontMetrics fm(font_);
    charWidth_ = fm.horizontalAdvance('M');
    charHeight_ = fm.height();
    ascent_ = fm.ascent();

    scrollY_ = 0;
    gutterPadding_ = 20;
    updateGutterWidth();

    modified_ = false;
    cursorVisible_ = true;

    blinkTimer_ = new QTimer(this);
    connect(blinkTimer_, &QTimer::timeout, this, &EditorWidget::toggleCursorBlink);
    blinkTimer_->start(500);

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::IBeamCursor);

    rebuildCommentState();
}

// ── File Operations ──

bool EditorWidget::openFile(const QString& path) {
    if (!buffer_.loadFromFile(path.toStdString())) return false;
    filePath_ = path;
    cursor_ = {0, 0};
    scrollY_ = 0;
    selection_.clear();
    undoManager_.clear();
    setModified(false);
    updateGutterWidth();
    rebuildCommentState();
    update();
    return true;
}

bool EditorWidget::saveFile() {
    if (filePath_.isEmpty()) return false;
    return saveFileAs(filePath_);
}

bool EditorWidget::saveFileAs(const QString& path) {
    if (!buffer_.saveToFile(path.toStdString())) return false;
    filePath_ = path;
    setModified(false);
    return true;
}

QString EditorWidget::filePath() const { return filePath_; }
bool EditorWidget::isModified() const { return modified_; }
QString EditorWidget::fileName() const {
    if (filePath_.isEmpty()) return "untitled";
    int sep = std::max(filePath_.lastIndexOf('/'), filePath_.lastIndexOf('\\'));
    return filePath_.mid(sep + 1);
}

int EditorWidget::currentRow() const { return cursor_.row; }
int EditorWidget::currentCol() const { return cursor_.col; }
int EditorWidget::totalLines() const { return buffer_.lineCount(); }
int EditorWidget::firstVisibleRow() const { return scrollY_ / charHeight_; }
int EditorWidget::lastVisibleRow() const {
    return std::min(firstVisibleRow() + height() / charHeight_ + 1, buffer_.lineCount() - 1);
}
int EditorWidget::getGutterWidth() const { return gutterWidth_; }
int EditorWidget::getCharHeight() const { return charHeight_; }
int EditorWidget::getScrollY() const { return scrollY_; }
int EditorWidget::getCharWidth() const { return charWidth_; }

void EditorWidget::setModified(bool m) {
    if (modified_ != m) {
        modified_ = m;
        emit modifiedChanged(m);
    }
}

void EditorWidget::updateGutterWidth() {
    int digits = 1;
    int lines = buffer_.lineCount();
    while (lines >= 10) { digits++; lines /= 10; }
    digits = std::max(digits, 3); // minimum 3 digits wide
    gutterWidth_ = digits * charWidth_ + gutterPadding_ * 2;
}

void EditorWidget::resetCursorBlink() {
    cursorVisible_ = true;
    blinkTimer_->start(500);
}

void EditorWidget::toggleCursorBlink() {
    cursorVisible_ = !cursorVisible_;
    // Only repaint the cursor region for performance
    update();
}

void EditorWidget::rebuildCommentState() {
    blockCommentState_.resize(buffer_.lineCount(), false);
    bool inComment = false;
    for (int i = 0; i < buffer_.lineCount(); i++) {
        blockCommentState_[i] = inComment;
        highlighter_.tokenize(buffer_.line(i), inComment);
    }
}

// ── Coordinate Conversion ──

int EditorWidget::rowFromY(int y) const {
    return std::clamp((y + scrollY_) / charHeight_, 0, buffer_.lineCount() - 1);
}

int EditorWidget::colFromX(int x, int row) const {
    int col = (x - gutterWidth_) / charWidth_;
    return std::clamp(col, 0, buffer_.lineLength(row));
}

int EditorWidget::xFromCol(int col) const {
    return gutterWidth_ + col * charWidth_;
}

int EditorWidget::yFromRow(int row) const {
    return row * charHeight_ - scrollY_;
}

int EditorWidget::maxScrollY() const {
    int totalHeight = buffer_.lineCount() * charHeight_;
    return std::max(0, totalHeight - height() + charHeight_);
}

void EditorWidget::clampScroll() {
    scrollY_ = std::clamp(scrollY_, 0, maxScrollY());
}

void EditorWidget::ensureCursorVisible() {
    int cy = cursor_.row * charHeight_;
    if (cy < scrollY_) {
        scrollY_ = cy;
    } else if (cy + charHeight_ > scrollY_ + height()) {
        scrollY_ = cy + charHeight_ - height();
    }
    clampScroll();
}

// ── Selection Helpers ──

void EditorWidget::updateSelectionForMove(bool shift) {
    if (shift) {
        if (!selection_.active) {
            selection_.start(cursor_);
        }
    } else {
        selection_.clear();
    }
}

void EditorWidget::deleteSelection() {
    if (!selection_.hasSelection(cursor_)) return;
    auto [start, end] = selection_.normalized(cursor_);

    std::string text = buffer_.getText(start, end);
    undoManager_.recordDelete(start, text);
    undoManager_.forceNewGroup();

    buffer_.deleteRange(start, end);
    cursor_ = start;
    selection_.clear();
    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

std::string EditorWidget::getSelectedText() const {
    if (!selection_.hasSelection(cursor_)) return "";
    auto [start, end] = selection_.normalized(cursor_);
    return buffer_.getText(start, end);
}

// ── Cursor Movement ──

void EditorWidget::moveCursorLeft(bool shift) {
    if (!shift && selection_.hasSelection(cursor_)) {
        auto [start, end] = selection_.normalized(cursor_);
        cursor_ = start;
        selection_.clear();
        return;
    }
    updateSelectionForMove(shift);
    if (cursor_.col > 0) {
        cursor_.col--;
    } else if (cursor_.row > 0) {
        cursor_.row--;
        cursor_.col = buffer_.lineLength(cursor_.row);
    }
}

void EditorWidget::moveCursorRight(bool shift) {
    if (!shift && selection_.hasSelection(cursor_)) {
        auto [start, end] = selection_.normalized(cursor_);
        cursor_ = end;
        selection_.clear();
        return;
    }
    updateSelectionForMove(shift);
    if (cursor_.col < buffer_.lineLength(cursor_.row)) {
        cursor_.col++;
    } else if (cursor_.row < buffer_.lineCount() - 1) {
        cursor_.row++;
        cursor_.col = 0;
    }
}

void EditorWidget::moveCursorUp(bool shift) {
    updateSelectionForMove(shift);
    if (cursor_.row > 0) {
        cursor_.row--;
        cursor_.col = std::min(cursor_.col, buffer_.lineLength(cursor_.row));
    }
}

void EditorWidget::moveCursorDown(bool shift) {
    updateSelectionForMove(shift);
    if (cursor_.row < buffer_.lineCount() - 1) {
        cursor_.row++;
        cursor_.col = std::min(cursor_.col, buffer_.lineLength(cursor_.row));
    }
}

void EditorWidget::moveCursorHome(bool shift, bool ctrl) {
    updateSelectionForMove(shift);
    if (ctrl) {
        cursor_ = {0, 0};
    } else {
        cursor_.col = 0;
    }
}

void EditorWidget::moveCursorEnd(bool shift, bool ctrl) {
    updateSelectionForMove(shift);
    if (ctrl) {
        cursor_.row = buffer_.lineCount() - 1;
        cursor_.col = buffer_.lineLength(cursor_.row);
    } else {
        cursor_.col = buffer_.lineLength(cursor_.row);
    }
}

void EditorWidget::moveCursorWordLeft(bool shift) {
    updateSelectionForMove(shift);
    if (cursor_.col == 0 && cursor_.row > 0) {
        cursor_.row--;
        cursor_.col = buffer_.lineLength(cursor_.row);
    } else {
        cursor_.col = buffer_.findWordBoundaryLeft(cursor_.row, cursor_.col);
    }
}

void EditorWidget::moveCursorWordRight(bool shift) {
    updateSelectionForMove(shift);
    if (cursor_.col >= buffer_.lineLength(cursor_.row) && cursor_.row < buffer_.lineCount() - 1) {
        cursor_.row++;
        cursor_.col = 0;
    } else {
        cursor_.col = buffer_.findWordBoundaryRight(cursor_.row, cursor_.col);
    }
}

// ── Indentation & Bracket Intelligence ──

char EditorWidget::closerFor(char open) {
    switch (open) {
        case '(':  return ')';
        case '[':  return ']';
        case '{':  return '}';
        case '"':  return '"';
        case '\'': return '\'';
        default:   return 0;
    }
}

bool EditorWidget::isCloser(char c) {
    return c == ')' || c == ']' || c == '}' || c == '"' || c == '\'';
}

bool EditorWidget::isIdentLike(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

int EditorWidget::indentWidthOf(const std::string& line) const {
    int width = 0;
    for (char c : line) {
        if (c == ' ')       width++;
        else if (c == '\t') width += INDENT_WIDTH - (width % INDENT_WIDTH);
        else break;
    }
    return width;
}

bool EditorWidget::onlyWhitespaceBefore(int row, int col) const {
    const std::string& line = buffer_.line(row);
    for (int i = 0; i < col && i < (int)line.size(); i++) {
        if (line[i] != ' ' && line[i] != '\t') return false;
    }
    return true;
}

bool EditorWidget::findMatchingOpenBrace(Position closePos, Position& out) const {
    int depth = 0;
    for (int row = closePos.row; row >= 0; row--) {
        const std::string& line = buffer_.line(row);

        // Braces inside a string literal or a comment are not structure. Reuse
        // the highlighter so this agrees with what the user actually sees.
        bool inComment = (row < (int)blockCommentState_.size()) ? blockCommentState_[row] : false;
        auto tokens = highlighter_.tokenize(line, inComment);
        std::vector<bool> isCode(line.size(), false);
        for (const auto& tok : tokens) {
            bool code = (tok.type != TokenType::String && tok.type != TokenType::Comment);
            for (int i = 0; i < tok.length; i++) {
                int c = tok.start + i;
                if (c >= 0 && c < (int)line.size()) isCode[c] = code;
            }
        }

        int startCol = (row == closePos.row) ? closePos.col - 1 : (int)line.size() - 1;
        for (int c = std::min(startCol, (int)line.size() - 1); c >= 0; c--) {
            if (!isCode[c]) continue;
            if (line[c] == '}') {
                depth++;
            } else if (line[c] == '{') {
                if (depth == 0) { out = {row, c}; return true; }
                depth--;
            }
        }
    }
    return false;
}

void EditorWidget::reindentClosingBrace() {
    int row = cursor_.row;
    int bracePos = cursor_.col - 1;          // cursor sits just after the '}'
    if (bracePos < 0) return;
    const std::string& line = buffer_.line(row);
    if (bracePos >= (int)line.size() || line[bracePos] != '}') return;

    // Only realign a brace that opens its own line. A '}' typed after code
    // (`} else {`, `int a[] = {1};`) must be left exactly where it was typed.
    if (!onlyWhitespaceBefore(row, bracePos)) return;

    Position openPos;
    int desired;
    if (findMatchingOpenBrace({row, bracePos}, openPos)) {
        desired = indentWidthOf(buffer_.line(openPos.row));
    } else {
        // Unbalanced source — fall back to one level shallower than we are.
        desired = std::max(0, indentWidthOf(line) - INDENT_WIDTH);
    }

    std::string desiredWs(desired, ' ');
    if ((int)desiredWs.size() == bracePos && line.compare(0, bracePos, desiredWs) == 0) {
        return;                               // already correct
    }

    Position start{row, 0};
    Position end{row, bracePos};
    std::string removed = buffer_.getText(start, end);
    if (!removed.empty()) {
        undoManager_.recordDelete(start, removed);
        buffer_.deleteRange(start, end);
    }
    if (!desiredWs.empty()) {
        undoManager_.recordInsert(start, desiredWs);
        buffer_.insertText(row, 0, desiredWs);
    }
    cursor_.col = desired + 1;                // immediately after the '}'
}

void EditorWidget::adjustColAfterIndent(int row, int delta) {
    if (cursor_.row == row) cursor_.col = std::max(0, cursor_.col + delta);
    if (selection_.active && selection_.anchor.row == row) {
        selection_.anchor.col = std::max(0, selection_.anchor.col + delta);
    }
}

void EditorWidget::indentBlock(int firstRow, int lastRow, bool unindent) {
    // One undo step for the whole block, so Ctrl+Z undoes the indent the user
    // applied rather than unpicking it a line at a time.
    undoManager_.forceNewGroup();

    for (int row = firstRow; row <= lastRow && row < buffer_.lineCount(); row++) {
        const std::string& line = buffer_.line(row);

        if (unindent) {
            int strip = 0;
            while (strip < INDENT_WIDTH && strip < (int)line.size() && line[strip] == ' ') strip++;
            if (strip == 0 && !line.empty() && line[0] == '\t') strip = 1;
            if (strip == 0) continue;

            Position s{row, 0}, e{row, strip};
            undoManager_.recordDelete(s, buffer_.getText(s, e));
            buffer_.deleteRange(s, e);
            adjustColAfterIndent(row, -strip);
        } else {
            if (line.empty()) continue;       // don't leave whitespace on blank lines
            std::string pad(INDENT_WIDTH, ' ');
            undoManager_.recordInsert({row, 0}, pad);
            buffer_.insertText(row, 0, pad);
            adjustColAfterIndent(row, INDENT_WIDTH);
        }
    }

    undoManager_.forceNewGroup();
}

// ── Edit Operations ──

void EditorWidget::handleChar(char ch) {
    if (selection_.hasSelection(cursor_)) deleteSelection();

    const std::string& line = buffer_.line(cursor_.row);
    char nextCh = (cursor_.col < (int)line.size()) ? line[cursor_.col] : '\0';
    char prevCh = (cursor_.col > 0) ? line[cursor_.col - 1] : '\0';

    // Typing a closer that is already sitting under the cursor steps over it
    // instead of doubling it. Without this, auto-close makes ')' unusable.
    if (isCloser(ch) && nextCh == ch) {
        cursor_.col++;
        return;
    }

    buffer_.insertChar(cursor_.row, cursor_.col, ch);
    undoManager_.recordInsert(cursor_, std::string(1, ch));
    cursor_.col++;

    if (ch == '}') {
        reindentClosingBrace();
    } else if (char closing = closerFor(ch)) {
        // Auto-closing into the middle of a word is never what was meant, and
        // "don't" must not become "don''t".
        bool suppress = isIdentLike(nextCh) ||
                        ((ch == '"' || ch == '\'') && isIdentLike(prevCh));
        if (!suppress) {
            buffer_.insertChar(cursor_.row, cursor_.col, closing);
            undoManager_.recordInsert({cursor_.row, cursor_.col}, std::string(1, closing));
            // Cursor stays between the pair
        }
    }

    setModified(true);
    rebuildCommentState();
}

void EditorWidget::handleBackspace(bool ctrl) {
    if (selection_.hasSelection(cursor_)) {
        deleteSelection();
        return;
    }

    if (ctrl) {
        // Delete word backward
        int newCol = buffer_.findWordBoundaryLeft(cursor_.row, cursor_.col);
        if (newCol == cursor_.col && cursor_.col == 0 && cursor_.row > 0) {
            // Merge with previous line
            int prevLen = buffer_.lineLength(cursor_.row - 1);
            std::string deleted = "\n";
            undoManager_.recordDelete({cursor_.row - 1, prevLen}, deleted);
            undoManager_.forceNewGroup();
            buffer_.mergeLines(cursor_.row);
            cursor_.row--;
            cursor_.col = prevLen;
        } else {
            Position start = {cursor_.row, newCol};
            Position end = cursor_;
            std::string text = buffer_.getText(start, end);
            undoManager_.recordDelete(start, text);
            undoManager_.forceNewGroup();
            buffer_.deleteRange(start, end);
            cursor_.col = newCol;
        }
    } else {
        if (cursor_.col > 0) {
            const std::string& l = buffer_.line(cursor_.row);
            char left  = l[cursor_.col - 1];
            char right = (cursor_.col < (int)l.size()) ? l[cursor_.col] : '\0';

            // Backspacing out of an empty pair the editor auto-inserted removes
            // both halves; leaving the orphaned closer behind is the single
            // most irritating failure mode of auto-close.
            if (right != '\0' && closerFor(left) == right) {
                Position start{cursor_.row, cursor_.col - 1};
                Position end{cursor_.row, cursor_.col + 1};
                undoManager_.recordDelete(start, buffer_.getText(start, end));
                undoManager_.forceNewGroup();
                buffer_.deleteRange(start, end);
                cursor_.col--;
                setModified(true);
                updateGutterWidth();
                rebuildCommentState();
                return;
            }

            std::string ch(1, left);
            undoManager_.recordDelete({cursor_.row, cursor_.col - 1}, ch);
            buffer_.deleteChar(cursor_.row, cursor_.col);
            cursor_.col--;
        } else if (cursor_.row > 0) {
            int prevLen = buffer_.lineLength(cursor_.row - 1);
            undoManager_.recordDelete({cursor_.row - 1, prevLen}, "\n");
            undoManager_.forceNewGroup();
            buffer_.mergeLines(cursor_.row);
            cursor_.row--;
            cursor_.col = prevLen;
        }
    }

    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

void EditorWidget::handleDelete() {
    if (selection_.hasSelection(cursor_)) {
        deleteSelection();
        return;
    }

    if (cursor_.col < buffer_.lineLength(cursor_.row)) {
        std::string ch(1, buffer_.line(cursor_.row)[cursor_.col]);
        undoManager_.recordDelete(cursor_, ch);
        // Delete forward: erase char at cursor position
        std::string& line = const_cast<std::string&>(buffer_.line(cursor_.row));
        line.erase(cursor_.col, 1);
    } else if (cursor_.row < buffer_.lineCount() - 1) {
        undoManager_.recordDelete(cursor_, "\n");
        undoManager_.forceNewGroup();
        // mergeLines(row+1) appends line[row+1] to line[row] and erases line[row+1]
        buffer_.mergeLines(cursor_.row + 1);
    }

    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

void EditorWidget::handleEnter() {
    if (selection_.hasSelection(cursor_)) deleteSelection();

    std::string indent = buffer_.getLeadingWhitespace(cursor_.row);

    // Look at the last real character before the cursor, not the character
    // immediately before it — otherwise `if (x) {` + Enter fails to indent,
    // because auto-close has left the cursor sitting between '{' and '}'.
    const std::string& currentLine = buffer_.line(cursor_.row);
    char lastCode = '\0';
    for (int i = std::min(cursor_.col, (int)currentLine.size()) - 1; i >= 0; i--) {
        if (currentLine[i] != ' ' && currentLine[i] != '\t') { lastCode = currentLine[i]; break; }
    }
    char nextCode = '\0';
    for (int i = cursor_.col; i < (int)currentLine.size(); i++) {
        if (currentLine[i] != ' ' && currentLine[i] != '\t') { nextCode = currentLine[i]; break; }
    }

    bool opensBlock = (lastCode == '{');
    bool isBetweenBraces = opensBlock && (nextCode == '}');

    std::string nextLineIndent = indent;
    if (opensBlock) {
        nextLineIndent += std::string(INDENT_WIDTH, ' ');
    }

    if (isBetweenBraces) {
        std::string textToInsert = "\n" + nextLineIndent + "\n" + indent;
        undoManager_.recordInsert(cursor_, textToInsert);
        undoManager_.forceNewGroup();
        
        buffer_.insertText(cursor_.row, cursor_.col, textToInsert);
        
        // Place cursor on the newly created middle line
        cursor_.row++;
        cursor_.col = nextLineIndent.length();
    } else {
        std::string textToInsert = "\n" + nextLineIndent;
        undoManager_.recordInsert(cursor_, textToInsert);
        undoManager_.forceNewGroup();
        
        cursor_ = buffer_.insertText(cursor_.row, cursor_.col, textToInsert);
    }

    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

void EditorWidget::handleTab(bool shift) {
    if (selection_.hasSelection(cursor_)) {
        auto [start, end] = selection_.normalized(cursor_);
        int lastRow = end.row;
        // A selection ending at column 0 stops short of that line; indenting it
        // would shift a line the user never highlighted.
        if (end.col == 0 && lastRow > start.row) lastRow--;

        if (shift || lastRow > start.row) {
            indentBlock(start.row, lastRow, shift);
            setModified(true);
            updateGutterWidth();
            rebuildCommentState();
            return;                           // selection survives, so Tab repeats
        }
        deleteSelection();                    // single-line selection: replace it
    } else if (shift) {
        indentBlock(cursor_.row, cursor_.row, true);
        setModified(true);
        rebuildCommentState();
        return;
    }

    std::string spaces(INDENT_WIDTH, ' ');
    Position tabStart = cursor_;              // record position BEFORE inserting
    buffer_.insertText(cursor_.row, cursor_.col, spaces);
    cursor_.col += INDENT_WIDTH;
    undoManager_.recordInsert(tabStart, spaces);
    undoManager_.forceNewGroup();
    setModified(true);
    rebuildCommentState();
}

void EditorWidget::movePage(int direction, bool shift) {
    updateSelectionForMove(shift);
    int rows = std::max(1, height() / charHeight_ - 1);
    cursor_.row = std::clamp(cursor_.row + direction * rows, 0, buffer_.lineCount() - 1);
    cursor_.col = std::min(cursor_.col, buffer_.lineLength(cursor_.row));
    scrollY_ = std::clamp(scrollY_ + direction * rows * charHeight_, 0, maxScrollY());
}

// ── Clipboard ──

void EditorWidget::copy() {
    std::string text = getSelectedText();
    if (!text.empty()) {
        QApplication::clipboard()->setText(QString::fromStdString(text));
    }
}

void EditorWidget::cut() {
    copy();
    if (selection_.hasSelection(cursor_)) {
        deleteSelection();
    }
}

void EditorWidget::paste() {
    QString clipText = QApplication::clipboard()->text();
    if (clipText.isEmpty()) return;

    if (selection_.hasSelection(cursor_)) deleteSelection();

    std::string text = clipText.toStdString();
    undoManager_.recordInsert(cursor_, text);
    undoManager_.forceNewGroup();

    Position newPos = buffer_.insertText(cursor_.row, cursor_.col, text);
    cursor_ = newPos;

    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

void EditorWidget::selectAll() {
    selection_.anchor = {0, 0};
    selection_.active = true;
    cursor_.row = buffer_.lineCount() - 1;
    cursor_.col = buffer_.lineLength(cursor_.row);
}

void EditorWidget::setSelection(int startRow, int startCol, int endRow, int endCol) {
    selection_.anchor = {startRow, startCol};
    selection_.active = true;
    cursor_ = {endRow, endCol};
    ensureCursorVisible();
    resetCursorBlink();
    emit cursorPositionChanged(cursor_.row, cursor_.col);
    update();
}

// ── Undo / Redo ──

void EditorWidget::performUndo() {
    auto result = undoManager_.undo();
    if (!result.valid) return;

    for (auto& action : result.actions) {
        if (action.type == EditAction::Insert) {
            // Undo insert = delete the text
            // The text was inserted at action.pos, so delete from action.pos for text.length
            Position start = action.pos;
            // Calculate end position from text
            int row = start.row, col = start.col;
            for (char c : action.text) {
                if (c == '\n') { row++; col = 0; }
                else col++;
            }
            Position endPos = {row, col};
            buffer_.deleteRange(start, endPos);
            cursor_ = start;
        } else {
            // Undo delete = re-insert the text
            buffer_.insertText(action.pos.row, action.pos.col, action.text);
            // Position cursor at end of re-inserted text
            int row = action.pos.row, col = action.pos.col;
            for (char c : action.text) {
                if (c == '\n') { row++; col = 0; }
                else col++;
            }
            cursor_ = {row, col};
        }
    }

    selection_.clear();
    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

void EditorWidget::performRedo() {
    auto result = undoManager_.redo();
    if (!result.valid) return;

    for (auto& action : result.actions) {
        if (action.type == EditAction::Insert) {
            Position newPos = buffer_.insertText(action.pos.row, action.pos.col, action.text);
            cursor_ = newPos;
        } else {
            Position start = action.pos;
            int row = start.row, col = start.col;
            for (char c : action.text) {
                if (c == '\n') { row++; col = 0; }
                else col++;
            }
            buffer_.deleteRange(start, {row, col});
            cursor_ = start;
        }
    }

    selection_.clear();
    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

// ── Input Handling ──

bool EditorWidget::event(QEvent* e) {
    // Intercept Tab/Backtab before Qt uses them for focus navigation
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
            keyPressEvent(ke);
            return true;
        }
    }
    return QWidget::event(e);
}

void EditorWidget::keyPressEvent(QKeyEvent* e) {
    bool ctrl = e->modifiers() & Qt::ControlModifier;
    bool shift = e->modifiers() & Qt::ShiftModifier;

    if (ctrl) {
        // ── Ctrl+ shortcuts ──
        switch (e->key()) {
        case Qt::Key_Z:     if (shift) performRedo(); else performUndo(); break;
        case Qt::Key_Y:     performRedo(); break;
        case Qt::Key_C:     copy(); break;
        case Qt::Key_X:     cut(); break;
        case Qt::Key_V:     paste(); break;
        case Qt::Key_A:     selectAll(); break;
        case Qt::Key_S:     emit saveRequested(); break;
        case Qt::Key_Left:  moveCursorWordLeft(shift); break;
        case Qt::Key_Right: moveCursorWordRight(shift); break;
        case Qt::Key_Home:  moveCursorHome(shift, true); break;
        case Qt::Key_End:   moveCursorEnd(shift, true); break;
        case Qt::Key_Backspace: handleBackspace(true); break;
        default: QWidget::keyPressEvent(e); return;
        }
    } else {
        // ── Normal keys ──
        switch (e->key()) {
        case Qt::Key_Left:      moveCursorLeft(shift); break;
        case Qt::Key_Right:     moveCursorRight(shift); break;
        case Qt::Key_Up:        moveCursorUp(shift); break;
        case Qt::Key_Down:      moveCursorDown(shift); break;
        case Qt::Key_Home:      moveCursorHome(shift, false); break;
        case Qt::Key_End:       moveCursorEnd(shift, false); break;
        case Qt::Key_Backspace: handleBackspace(false); break;
        case Qt::Key_Delete:    handleDelete(); break;
        case Qt::Key_Return:
        case Qt::Key_Enter:     handleEnter(); break;
        case Qt::Key_Tab:       handleTab(false); break;
        case Qt::Key_Backtab:   handleTab(true); break;
        case Qt::Key_PageUp:    movePage(-1, shift); break;
        case Qt::Key_PageDown:  movePage(1, shift); break;
        default:
            if (!e->text().isEmpty()) {
                QChar ch = e->text().at(0);
                // TextBuffer is byte-indexed and the renderer assumes one byte
                // per column, so a non-ASCII character cannot be represented
                // without desynchronising every column calculation. Drop it
                // rather than inserting the NUL byte toLatin1() would produce.
                if (ch.isPrint() && ch.unicode() < 128) {
                    handleChar(static_cast<char>(ch.unicode()));
                }
            }
            break;
        }
    }

    ensureCursorVisible();
    resetCursorBlink();
    emit cursorPositionChanged(cursor_.row, cursor_.col);
    update();
}

// ── Mouse Handling ──

void EditorWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && e->position().x() > gutterWidth_) {
        int row = rowFromY((int)e->position().y());
        int col = colFromX((int)e->position().x(), row);
        cursor_ = {row, col};
        selection_.clear();
        selection_.start(cursor_);
        resetCursorBlink();
        emit cursorPositionChanged(cursor_.row, cursor_.col);
        update();
    }
}

void EditorWidget::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton && e->position().x() > gutterWidth_) {
        int row = rowFromY((int)e->position().y());
        int col = colFromX((int)e->position().x(), row);
        cursor_ = {row, col};
        ensureCursorVisible();
        emit cursorPositionChanged(cursor_.row, cursor_.col);
        update();
    }
}

void EditorWidget::wheelEvent(QWheelEvent* e) {
    int delta = e->angleDelta().y();
    int lines = 3;
    scrollY_ -= (delta / 120) * lines * charHeight_;
    clampScroll();
    update();
}

void EditorWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    clampScroll();
}

// ── Paint ──

void EditorWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setFont(font_);

    // Background
    p.fillRect(rect(), Theme::EditorBg);

    // Visible rows
    int startRow = scrollY_ / charHeight_;
    int endRow = std::min(startRow + height() / charHeight_ + 2, buffer_.lineCount());

    paintGutter(p, startRow, endRow);
    paintCode(p, startRow, endRow);
    paintCursor(p);
}

void EditorWidget::paintGutter(QPainter& p, int startRow, int endRow) {
    // Gutter background
    p.fillRect(0, 0, gutterWidth_, height(), Theme::SidebarBg);

    // Gutter border
    p.setPen(Theme::Border);
    p.drawLine(gutterWidth_ - 1, 0, gutterWidth_ - 1, height());

    p.setFont(font_);
    for (int row = startRow; row < endRow; row++) {
        int y = yFromRow(row);
        if (row == cursor_.row) {
            p.setPen(Theme::TextPrimary);
        } else {
            p.setPen(Theme::TextMuted);
        }
        QString num = QString::number(row + 1);
        int x = gutterWidth_ - gutterPadding_ - p.fontMetrics().horizontalAdvance(num);
        p.drawText(x, y + ascent_, num);
    }
}

void EditorWidget::paintCode(QPainter& p, int startRow, int endRow) {
    p.setFont(font_);

    for (int row = startRow; row < endRow; row++) {
        int y = yFromRow(row);

        // Current line highlight
        if (row == cursor_.row) {
            p.fillRect(gutterWidth_, y, width() - gutterWidth_, charHeight_, Theme::CurrentLine);
        }

        // Selection highlight
        paintSelection(p, row, y);

        // Tokenize and draw
        bool inComment = (row < (int)blockCommentState_.size()) ? blockCommentState_[row] : false;
        const std::string& lineStr = buffer_.line(row);
        auto tokens = highlighter_.tokenize(lineStr, inComment);

        for (const auto& tok : tokens) {
            QColor color;
            switch (tok.type) {
            case TokenType::Keyword:      color = Theme::SynKeyword; break;
            case TokenType::Type:         color = Theme::SynType; break;
            case TokenType::String:       color = Theme::SynString; break;
            case TokenType::Comment:      color = Theme::SynComment; break;
            case TokenType::Number:       color = Theme::SynNumber; break;
            case TokenType::Preprocessor: color = Theme::SynPreprocessor; break;
            case TokenType::Function:     color = Theme::SynFunction; break;
            case TokenType::Punctuation:  color = Theme::SynPunctuation; break;
            default:                      color = Theme::TextPrimary; break;
            }

            p.setPen(color);
            // Draw character by character to enforce a strict monospace grid.
            // This prevents "ghost characters" or cursor drift if the system
            // falls back to a proportional font or applies kerning.
            for (int i = 0; i < tok.length; i++) {
                int x = gutterWidth_ + (tok.start + i) * charWidth_;
                QString ch = QString::fromStdString(lineStr.substr(tok.start + i, 1));
                p.drawText(x, y + ascent_, ch);
            }
        }
    }
}

void EditorWidget::paintSelection(QPainter& p, int row, int y) {
    if (!selection_.hasSelection(cursor_)) return;

    auto [selStart, selEnd] = selection_.normalized(cursor_);
    if (row < selStart.row || row > selEnd.row) return;

    int x1, x2;
    if (row == selStart.row && row == selEnd.row) {
        x1 = xFromCol(selStart.col);
        x2 = xFromCol(selEnd.col);
    } else if (row == selStart.row) {
        x1 = xFromCol(selStart.col);
        x2 = xFromCol(buffer_.lineLength(row)) + charWidth_;
    } else if (row == selEnd.row) {
        x1 = gutterWidth_;
        x2 = xFromCol(selEnd.col);
    } else {
        x1 = gutterWidth_;
        x2 = xFromCol(buffer_.lineLength(row)) + charWidth_;
    }

    p.fillRect(x1, y, x2 - x1, charHeight_, Theme::SelectionBg);
}

void EditorWidget::paintCursor(QPainter& p) {
    if (!cursorVisible_) return;
    if (cursor_.row < firstVisibleRow() || cursor_.row > lastVisibleRow()) return;

    int cx = xFromCol(cursor_.col);
    int cy = yFromRow(cursor_.row);

    // Glow
    p.fillRect(cx - 2, cy, 6, charHeight_, Theme::AccentGlow);
    // Cursor line
    p.fillRect(cx, cy, 2, charHeight_, Theme::Accent);
}
