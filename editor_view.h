#pragma once

#include <QWidget>
#include <QFont>
#include <QTimer>
#include "text_buffer.h"

struct Cursor {
    int row = 0;
    int col = 0;
};

class EditorView : public QWidget {
    Q_OBJECT

public:
    explicit EditorView(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    TextBuffer buffer;
    Cursor cursor;

    QFont font;
    int charWidth;
    int charHeight;

    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorUp();
    void moveCursorDown();
};
