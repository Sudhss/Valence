#include "window.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

Window::Window(QWidget *parent) : QWidget(parent)
{
    cursorTimer = new QTimer(this);

    connect(cursorTimer, &QTimer::timeout, this, [this]() {
        cursorVisible = !cursorVisible;
        update();
    });

    cursorTimer->start(500);
}

void Window::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    QFont font;
    font.setPointSize(20);
    painter.setFont(font);

    int baseX = 10;
    int baseY = 20;
    pos = (char)baseX;
    painter.drawText(baseX, baseY, text);
    QFontMetrics fm(font);
    int cursorX = baseX + fm.horizontalAdvance(text.left(cursorPos));
    painter.drawText(200,300,pos);

    if (cursorVisible)
    {
        painter.drawLine(
            cursorX,
            baseY - fm.ascent(),
            cursorX,
            baseY + fm.descent()
            );
    }
}

void Window::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace &&
        (event->modifiers() & Qt::ControlModifier))
    {
        if (cursorPos > 0){
            int start = cursorPos;
            while (start > 0 && text[start - 1] == ' ')
                start--;
            while (start > 0 && text[start - 1] != ' ')
                start--;

            text.remove(start, cursorPos - start);
            cursorPos = start;
        }
    }
    else if (event->key() == Qt::Key_Backspace)
    {
        if (cursorPos > 0)
        {
            text.remove(cursorPos - 1, 1);
            cursorPos--;
        }
    }
    else if (event->key() == Qt::Key_Left)
    {
        if (cursorPos > 0)
            cursorPos--;
    }
    // Move Right
    else if (event->key() == Qt::Key_Right)
    {
        if (cursorPos < text.length())
            cursorPos++;
    }
    // Insert character
    else if (!event->text().isEmpty())
    {
        text.insert(cursorPos, event->text());
        cursorPos++;
    }

    // Reset blink on key press
    cursorVisible = true;
    cursorTimer->start(500);

    update();
}
