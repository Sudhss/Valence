#include "editor_view.h"
#include <QPainter>
#include <QKeyEvent>
#include <QFontMetrics>

EditorView::EditorView(QWidget* parent) : QWidget(parent) {
    font = QFont("Consolas", 12);
    font.setStyleHint(QFont::Monospace);

    QFontMetrics fm(font);
    charWidth = fm.horizontalAdvance('M');
    charHeight = fm.height();

    setFocusPolicy(Qt::StrongFocus);
}

void EditorView::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#0b0b0b"));
    painter.setFont(font);

    int y = charHeight;

    for (int i = 0; i < buffer.getLineCount(); i++) {
        QString line = QString::fromStdString(buffer.getLine(i));
        painter.setPen(QColor("#d4d4d4"));
        painter.drawText(10, y, line);
        y += charHeight;
    }

    // cursor
    int x = 10 + cursor.col * charWidth;
    int cy = cursor.row * charHeight;

    painter.fillRect(x, cy, 2, charHeight, QColor("#00ff9c"));
}

void EditorView::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Left: moveCursorLeft(); break;
    case Qt::Key_Right: moveCursorRight(); break;
    case Qt::Key_Up: moveCursorUp(); break;
    case Qt::Key_Down: moveCursorDown(); break;

    case Qt::Key_Backspace:
        buffer.deleteChar(cursor.row, cursor.col);
        if (cursor.col > 0) cursor.col--;
        else if (cursor.row > 0) {
            cursor.row--;
            cursor.col = buffer.getLineLength(cursor.row);
        }
        break;

    case Qt::Key_Return:
        buffer.splitLine(cursor.row, cursor.col);
        cursor.row++;
        cursor.col = 0;
        break;

    default:
        if (!event->text().isEmpty()) {
            char ch = event->text().toStdString()[0];
            buffer.insertChar(cursor.row, cursor.col, ch);
            cursor.col++;
        }
    }

    update();
}

void EditorView::moveCursorLeft() {
    if (cursor.col > 0) cursor.col--;
    else if (cursor.row > 0) {
        cursor.row--;
        cursor.col = buffer.getLineLength(cursor.row);
    }
}

void EditorView::moveCursorRight() {
    if (cursor.col < buffer.getLineLength(cursor.row)) cursor.col++;
    else if (cursor.row < buffer.getLineCount() - 1) {
        cursor.row++;
        cursor.col = 0;
    }
}

void EditorView::moveCursorUp() {
    if (cursor.row > 0) {
        cursor.row--;
        cursor.col = std::min(cursor.col, buffer.getLineLength(cursor.row));
    }
}

void EditorView::moveCursorDown() {
    if (cursor.row < buffer.getLineCount() - 1) {
        cursor.row++;
        cursor.col = std::min(cursor.col, buffer.getLineLength(cursor.row));
    }
}
