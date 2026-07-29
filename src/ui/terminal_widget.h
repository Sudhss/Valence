#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include <QProcess>

class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget();

    // Send a command to the terminal programmatically
    void runCommand(const QString& cmd);
    void clearOutput();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onReadyRead();

private:
    QPlainTextEdit* output_;
    QProcess* process_;
    int inputStartPosition_ = 0;
    void applyStyle();
};
