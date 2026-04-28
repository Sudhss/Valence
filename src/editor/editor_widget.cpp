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

// ── Edit Operations ──

void EditorWidget::handleChar(char ch) {
    if (selection_.hasSelection(cursor_)) deleteSelection();

    buffer_.insertChar(cursor_.row, cursor_.col, ch);
    undoManager_.recordInsert(cursor_, std::string(1, ch));
    cursor_.col++;
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
            std::string ch(1, buffer_.line(cursor_.row)[cursor_.col - 1]);
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
        // Delete forward: remove char at cursor position
        buffer_.line(cursor_.row); // just accessing
        // We need a deleteForward — use a direct erase
        std::string& line = const_cast<std::string&>(buffer_.line(cursor_.row));
        line.erase(cursor_.col, 1);
    } else if (cursor_.row < buffer_.lineCount() - 1) {
        undoManager_.recordDelete(cursor_, "\n");
        undoManager_.forceNewGroup();
        // Merge next line into current
        std::string& curLine = const_cast<std::string&>(buffer_.line(cursor_.row));
        curLine += buffer_.line(cursor_.row + 1);
        // Remove next line
        // We need direct access — this is a bit hacky but works for V1
        buffer_.mergeLines(cursor_.row + 1);
    }

    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

void EditorWidget::handleEnter() {
    if (selection_.hasSelection(cursor_)) deleteSelection();

    std::string indent = buffer_.getLeadingWhitespace(cursor_.row);

    undoManager_.recordInsert(cursor_, "\n" + indent);
    undoManager_.forceNewGroup();

    buffer_.splitLine(cursor_.row, cursor_.col);
    cursor_.row++;
    cursor_.col = 0;

    // Auto-indent: insert leading whitespace
    if (!indent.empty()) {
        for (char c : indent) {
            buffer_.insertChar(cursor_.row, cursor_.col, c);
            cursor_.col++;
        }
    }

    setModified(true);
    updateGutterWidth();
    rebuildCommentState();
}

void EditorWidget::handleTab() {
    if (selection_.hasSelection(cursor_)) deleteSelection();

    std::string spaces = "    "; // 4 spaces
    for (char c : spaces) {
        buffer_.insertChar(cursor_.row, cursor_.col, c);
        cursor_.col++;
    }
    undoManager_.recordInsert(cursor_, spaces);
    undoManager_.forceNewGroup();
    setModified(true);
    rebuildCommentState();
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

// ── Undo / Redo ──

void EditorWidget::performUndo() {
    auto result = undoManager_.undo();
    if (!result.valid) return;

    for (auto& action : result.actions) {
        if (action.type == EditAction::Insert) {
            // Undo insert = delete the text
            Position end = buffer_.insertText(action.pos.row, action.pos.col, ""); // dummy
            // Actually: find end position and delete
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

void EditorWidget::keyPressEvent(QKeyEvent* e) {
    bool ctrl = e->modifiers() & Qt::ControlModifier;
    bool shift = e->modifiers() & Qt::ShiftModifier;

    switch (e->key()) {
    case Qt::Key_Left:
        if (ctrl) moveCursorWordLeft(shift);
        else moveCursorLeft(shift);
        break;
    case Qt::Key_Right:
        if (ctrl) moveCursorWordRight(shift);
        else moveCursorRight(shift);
        break;
    case Qt::Key_Up:    moveCursorUp(shift); break;
    case Qt::Key_Down:  moveCursorDown(shift); break;
    case Qt::Key_Home:  moveCursorHome(shift, ctrl); break;
    case Qt::Key_End:   moveCursorEnd(shift, ctrl); break;

    case Qt::Key_Backspace: handleBackspace(ctrl); break;
    case Qt::Key_Delete:    handleDelete(); break;
    case Qt::Key_Return:
    case Qt::Key_Enter:     handleEnter(); break;
    case Qt::Key_Tab:       handleTab(); break;

    case Qt::Key_Z:
        if (ctrl) { if (shift) performRedo(); else performUndo(); }
        break;
    case Qt::Key_Y:
        if (ctrl) performRedo();
        break;
    case Qt::Key_C:
        if (ctrl) copy();
        break;
    case Qt::Key_X:
        if (ctrl) cut();
        break;
    case Qt::Key_V:
        if (ctrl) paste();
        break;
    case Qt::Key_A:
        if (ctrl) selectAll();
        break;
    case Qt::Key_S:
        if (ctrl) { emit saveRequested(); return; }
        // Fall through to default for 's' character
        goto handleDefault;

    default:
    handleDefault:
        if (!e->text().isEmpty() && !ctrl) {
            QChar ch = e->text().at(0);
            if (ch.isPrint()) {
                handleChar(ch.toLatin1());
            }
        }
        break;
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
            int x = gutterWidth_ + tok.start * charWidth_;
            QString text = QString::fromStdString(lineStr.substr(tok.start, tok.length));
            p.drawText(x, y + ascent_, text);
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
