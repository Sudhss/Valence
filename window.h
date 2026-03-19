#pragma once
#include <QWidget>
#include <QKeyEvent>
#include <QString>
#include <QTimer>

class Window : public QWidget
{
public:
    Window(QWidget *parent = nullptr);
private:
    int cursorPos = 0;
    QString text = "";
    QString pos = 0;
    bool cursorVisible = true;
    QTimer *cursorTimer;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

};
