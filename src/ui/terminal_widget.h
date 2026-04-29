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

    // Send a command to the terminal programmatically
    void runCommand(const QString& cmd);
    void clearOutput();

private slots:
    void onReadyRead();
    void onCommandEntered();

private:
    QPlainTextEdit* output_;
    QLineEdit* input_;
    QProcess* process_;
    void applyStyle();
};
