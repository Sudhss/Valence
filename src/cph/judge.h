#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include "test_case.h"

class QProcess;
class QTimer;
class QElapsedTimer;
class QTemporaryDir;

// Compiles a single translation unit, then feeds each test case through the
// resulting binary and reports a verdict per case.
//
// Everything is asynchronous. No method on this class ever blocks the GUI
// thread — not for compilation, not for a runaway solution that never exits.
// Results arrive on the signals below, in order, one case at a time.
class Judge : public QObject {
    Q_OBJECT

public:
    explicit Judge(QObject* parent = nullptr);
    ~Judge() override;

    // ── Configuration ──
    void setTimeLimitMs(int ms);          // default 3000
    int  timeLimitMs() const;

    // Compiler invocation. Defaults to "g++" on PATH with -O2 -std=gnu++17.
    void setCompiler(const QString& exe);
    QString compiler() const;

    bool isBusy() const;

    // ── Driving ──
    // Compiles sourcePath, then runs every case in order. Emits failed() and
    // stops if the toolchain is missing or the source cannot be read.
    // Calling run() while busy is ignored.
    void run(const QString& sourcePath, const QVector<TestCase>& cases);

    // Kills whatever is in flight. Safe to call when idle. allFinished() is
    // NOT emitted for a cancelled run.
    void cancel();

signals:
    void compileStarted();
    void compileFinished(bool ok, const QString& compilerOutput);
    void caseStarted(int index);
    void caseFinished(int index, const TestCase& result);
    void allFinished(int passed, int total);
    void failed(const QString& reason);

public:
    // Output comparison used to decide Accepted vs WrongAnswer. Exposed so the
    // UI can highlight the first differing line with exactly the same rule the
    // verdict was computed from. Trailing whitespace on each line is ignored,
    // as are trailing blank lines; everything else must match exactly.
    static bool outputsMatch(const QString& actual, const QString& expected);

    // Index of the first line that differs under outputsMatch(), or -1 if they
    // match. Used to scroll the diff view to the interesting line.
    static int firstDifferingLine(const QString& actual, const QString& expected);

private:
    void startNextCase();
    void finishCase(Verdict verdict, int exitCode, const QString& errorText);

    // Implementation detail — the engine owns these. The public surface above
    // is frozen; add whatever private state the implementation needs.
    struct Impl;
    Impl* d;
};
