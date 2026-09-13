#include "judge.h"

#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QStringList>

namespace {

// Normalises output for comparison: trailing whitespace on each line is
// ignored, as are trailing blank lines. Everything else must match exactly,
// including case — judges are not lenient and neither is this.
QStringList normalizeOutput(const QString& text) {
    QStringList lines = text.split(QLatin1Char('\n'));
    for (QString& line : lines) {
        while (!line.isEmpty()) {
            const QChar c = line.at(line.size() - 1);
            if (c == QLatin1Char('\r') || c == QLatin1Char(' ') || c == QLatin1Char('\t')) {
                line.chop(1);
            } else {
                break;
            }
        }
    }
    while (!lines.isEmpty() && lines.constLast().isEmpty()) lines.removeLast();
    return lines;
}

} // namespace

struct Judge::Impl {
    QString compiler = QStringLiteral("g++");
    int timeLimitMs = 3000;

    bool busy = false;
    QTemporaryDir* tempDir = nullptr;
    QString binaryPath;
    int buildSerial = 0;            // fresh exe name per build; Windows holds locks

    QProcess* compileProc = nullptr;
    QProcess* runProc = nullptr;
    QTimer* timeoutTimer = nullptr;
    QElapsedTimer clock;

    QVector<TestCase> cases;
    int current = -1;
    int passed = 0;
    bool caseReported = false;      // guards the kill/finish double-delivery race
};

Judge::Judge(QObject* parent) : QObject(parent), d(new Impl) {
    d->timeoutTimer = new QTimer(this);
    d->timeoutTimer->setSingleShot(true);
}

Judge::~Judge() {
    // Never leave an orphaned child process behind. This is the one place a
    // bounded block is acceptable — we are tearing down, not drawing.
    auto reap = [](QProcess*& p) {
        if (!p) return;
        p->disconnect();
        if (p->state() != QProcess::NotRunning) {
            p->kill();
            p->waitForFinished(500);
        }
        delete p;
        p = nullptr;
    };
    reap(d->compileProc);
    reap(d->runProc);
    delete d->tempDir;
    delete d;
}

void Judge::setTimeLimitMs(int ms) { d->timeLimitMs = qMax(100, ms); }
int  Judge::timeLimitMs() const    { return d->timeLimitMs; }

void Judge::setCompiler(const QString& exe) {
    if (!exe.trimmed().isEmpty()) d->compiler = exe.trimmed();
}
QString Judge::compiler() const { return d->compiler; }

bool Judge::isBusy() const { return d->busy; }

// ── Comparison ───────────────────────────────────────────────────────────────

bool Judge::outputsMatch(const QString& actual, const QString& expected) {
    return normalizeOutput(actual) == normalizeOutput(expected);
}

int Judge::firstDifferingLine(const QString& actual, const QString& expected) {
    const QStringList a = normalizeOutput(actual);
    const QStringList e = normalizeOutput(expected);
    const int n = qMin(a.size(), e.size());
    for (int i = 0; i < n; i++) {
        if (a.at(i) != e.at(i)) return i;
    }
    if (a.size() != e.size()) return n;
    return -1;
}

// ── Driving ──────────────────────────────────────────────────────────────────

void Judge::run(const QString& sourcePath, const QVector<TestCase>& cases) {
    if (d->busy) return;

    const QFileInfo srcInfo(sourcePath);
    if (sourcePath.isEmpty() || !srcInfo.exists() || !srcInfo.isReadable()) {
        emit failed(tr("There is no saved source file to compile."));
        return;
    }
    if (cases.isEmpty()) {
        emit failed(tr("Add at least one test case first."));
        return;
    }

    const QString compilerPath = QStandardPaths::findExecutable(d->compiler);
    if (compilerPath.isEmpty()) {
        emit failed(tr("%1 was not found on your PATH. Install MinGW or add it to PATH.")
                        .arg(d->compiler));
        return;
    }

    if (!d->tempDir) {
        d->tempDir = new QTemporaryDir();
        if (!d->tempDir->isValid()) {
            delete d->tempDir;
            d->tempDir = nullptr;
            emit failed(tr("Could not create a temporary build directory."));
            return;
        }
    }

    // Reset per-run state.
    d->cases = cases;
    for (TestCase& tc : d->cases) {
        tc.verdict = Verdict::Pending;
        tc.actual.clear();
        tc.errorText.clear();
        tc.elapsedMs = 0;
        tc.exitCode = 0;
    }
    d->current = -1;
    d->passed = 0;
    d->busy = true;

    // A fresh name each build: on Windows the previous binary can still be
    // locked by a process that has only just exited.
    d->binaryPath = d->tempDir->filePath(QStringLiteral("solution_%1.exe")
                                             .arg(++d->buildSerial));

    d->compileProc = new QProcess(this);
    d->compileProc->setProcessChannelMode(QProcess::MergedChannels);

    QProcess* proc = d->compileProc;
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError err) {
        if (proc != d->compileProc) return;
        if (err != QProcess::FailedToStart) return;
        d->compileProc = nullptr;
        proc->disconnect(this);
        proc->deleteLater();
        d->busy = false;
        emit failed(tr("Could not start %1.").arg(d->compiler));
    });
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, proc](int exitCode, QProcess::ExitStatus) {
        if (proc != d->compileProc) return;
        const QString output = QString::fromLocal8Bit(proc->readAll());
        d->compileProc = nullptr;
        proc->disconnect(this);
        proc->deleteLater();

        const bool ok = (exitCode == 0);
        emit compileFinished(ok, output);
        if (!ok) {
            // The whole run is a compile error. Report it per case as well, so
            // the cards show CE rather than sitting on a stale verdict.
            for (int i = 0; i < d->cases.size(); i++) {
                d->cases[i].verdict = Verdict::CompileError;
                d->cases[i].errorText = output;
                emit caseFinished(i, d->cases.at(i));
            }
            d->busy = false;
            emit allFinished(0, d->cases.size());
            return;
        }
        startNextCase();
    });

    emit compileStarted();
    proc->start(compilerPath, {QStringLiteral("-O2"),
                               QStringLiteral("-std=gnu++17"),
                               QStringLiteral("-o"), d->binaryPath,
                               srcInfo.absoluteFilePath()});
}

void Judge::startNextCase() {
    d->current++;
    if (d->current >= d->cases.size()) {
        d->busy = false;
        emit allFinished(d->passed, d->cases.size());
        return;
    }

    const int index = d->current;
    d->caseReported = false;
    d->cases[index].verdict = Verdict::Running;
    emit caseStarted(index);

    QProcess* proc = new QProcess(this);
    d->runProc = proc;
    proc->setProcessChannelMode(QProcess::SeparateChannels);
    proc->setWorkingDirectory(QFileInfo(d->binaryPath).absolutePath());

    connect(proc, &QProcess::started, this, [this, proc]() {
        if (proc != d->runProc) return;
        QString input = d->cases.at(d->current).input;
        // Almost every solution reads with `cin >>` or getline; a missing final
        // newline makes the last token or line never arrive and the process
        // hangs until the time limit. Supply it.
        if (!input.isEmpty() && !input.endsWith(QLatin1Char('\n'))) {
            input.append(QLatin1Char('\n'));
        }
        proc->write(input.toUtf8());
        proc->closeWriteChannel();
    });

    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError err) {
        if (proc != d->runProc || d->caseReported) return;
        if (err == QProcess::FailedToStart) {
            finishCase(Verdict::RuntimeError, -1, tr("The compiled program could not be started."));
        }
        // Crashes arrive via finished() with CrashExit; nothing to do here.
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, proc](int exitCode, QProcess::ExitStatus status) {
        if (proc != d->runProc || d->caseReported) return;

        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        const QString err = QString::fromUtf8(proc->readAllStandardError());
        d->cases[d->current].actual = out;

        if (status == QProcess::CrashExit) {
            finishCase(Verdict::RuntimeError, exitCode,
                       err.isEmpty() ? tr("The program crashed.") : err);
        } else if (exitCode != 0) {
            // A non-zero exit outranks any output comparison: the run is invalid.
            finishCase(Verdict::RuntimeError, exitCode,
                       err.isEmpty() ? tr("Exited with code %1.").arg(exitCode) : err);
        } else if (outputsMatch(out, d->cases.at(d->current).expected)) {
            finishCase(Verdict::Accepted, exitCode, err);
        } else {
            finishCase(Verdict::WrongAnswer, exitCode, err);
        }
    });

    d->timeoutTimer->disconnect();
    connect(d->timeoutTimer, &QTimer::timeout, this, [this, proc]() {
        if (proc != d->runProc || d->caseReported) return;
        d->cases[d->current].actual = QString::fromUtf8(proc->readAllStandardOutput());
        finishCase(Verdict::TimeLimit, -1, tr("Exceeded %1 ms.").arg(d->timeLimitMs));
    });

    d->clock.start();
    d->timeoutTimer->start(d->timeLimitMs);
    proc->start(d->binaryPath, QStringList());
}

void Judge::finishCase(Verdict verdict, int exitCode, const QString& errorText) {
    if (d->caseReported) return;
    d->caseReported = true;
    d->timeoutTimer->stop();

    const int index = d->current;
    TestCase& tc = d->cases[index];
    tc.verdict = verdict;
    tc.exitCode = exitCode;
    tc.errorText = errorText;
    tc.elapsedMs = (verdict == Verdict::TimeLimit) ? d->timeLimitMs
                                                   : static_cast<int>(d->clock.elapsed());
    if (verdict == Verdict::Accepted) d->passed++;

    // Detach the process before advancing. A killed process still emits
    // finished(), and without this that stale signal would be attributed to the
    // NEXT case — the single nastiest bug lurking in this file.
    if (QProcess* proc = d->runProc) {
        d->runProc = nullptr;
        proc->disconnect(this);
        if (proc->state() != QProcess::NotRunning) proc->kill();
        proc->deleteLater();
    }

    emit caseFinished(index, tc);
    startNextCase();
}

void Judge::cancel() {
    if (!d->busy) return;
    d->busy = false;
    d->caseReported = true;          // suppress any in-flight completion
    d->timeoutTimer->stop();
    d->timeoutTimer->disconnect();

    auto stop = [this](QProcess*& p) {
        if (!p) return;
        QProcess* proc = p;
        p = nullptr;
        proc->disconnect(this);
        if (proc->state() != QProcess::NotRunning) proc->kill();
        proc->deleteLater();
    };
    stop(d->compileProc);
    stop(d->runProc);
    // Deliberately no allFinished() — a cancelled run has no result.
}
