#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QStringList>

class QLabel;

// The integrated terminal: a PowerShell child process with its output and an
// editable input region sharing one QPlainTextEdit.
//
// All I/O is signal-driven. Nothing here blocks the GUI thread.
class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget() override;

    // Send a command to the terminal programmatically
    void runCommand(const QString& cmd);
    void clearOutput();

    // Where the shell starts, and where it is moved to when the open folder
    // changes. Without this the shell inherits whatever directory the
    // application process happened to be launched from, which is never what
    // the user wants to see in a prompt.
    void setWorkingDirectory(const QString& dir);
    QString workingDirectory() const { return workingDir_; }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onReadyRead();

private:
    QPlainTextEdit* output_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QProcess* process_ = nullptr;

    // Where the editable region starts, expressed as a column within the LAST
    // block rather than an absolute document position. maximumBlockCount trims
    // lines off the top on a long build, which shifts every absolute position
    // and would otherwise silently corrupt the input guard.
    int inputColumn_ = 0;

    QString ansiPending_;          // a partial escape split across two reads
    QStringList history_;
    int historyIndex_ = 0;
    QString historyDraft_;         // what was typed before browsing history
    bool shellAlive_ = false;
    QString workingDir_;

    void applyStyle();
    void startShell();
    void appendOutput(const QString& text, const QColor& color = QColor());
    void markInputStart();

    int inputStart() const;
    QString currentInput() const;
    void replaceInput(const QString& text);
    void submitInput();
    bool atBottom() const;

    QString stripAnsi(const QString& chunk);
};
