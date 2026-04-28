#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QProcess>

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget();

private slots:
    void onReadyRead();
    void onCommandEntered();

private:
    QPlainTextEdit* output_;
    QLineEdit* input_;
    QProcess* process_;
    void applyStyle();
};
