#pragma once
#include <QWidget>
#include <QString>

// The Competitive Programming Helper panel.
//
// Lives in a QDockWidget on the right. Owns a list of test-case cards and a
// Judge; compiles the active file once and pipes each case through it,
// showing a pass/fail state per case.
//
// PUBLIC API IS FROZEN — MainWindow wires against exactly this. Private
// members and helper slots may be added freely.
class CphPanel : public QWidget {
    Q_OBJECT

public:
    explicit CphPanel(QWidget* parent = nullptr);
    ~CphPanel() override;

    // The .cpp the panel judges. Called whenever the active tab changes.
    // Passing an empty path puts the panel in its idle/empty state.
    // Switching files persists the current file's cases and loads the new
    // file's, so each problem keeps its own tests.
    void setTargetFile(const QString& path);
    QString targetFile() const;

    // Flush the in-memory cases for the current target to its sidecar file.
    // Called by MainWindow before the app closes.
    void persist();

public slots:
    void addTestCase();
    void runAll();
    void stopAll();

signals:
    // Emitted at the very start of runAll(), before the source is read.
    // MainWindow connects this to its save routine; the default (direct)
    // connection means the save completes before runAll() reads the file.
    void saveBeforeRunRequested();

    // Short human-readable progress for the status bar ("3/5 passed").
    void statusMessage(const QString& message);

private:
    struct Impl;
    Impl* d;
};
