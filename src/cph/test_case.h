#pragma once
#include <QString>

// A single competitive-programming test case and the result of judging it.
// Kept as a plain value type so the UI can copy it around freely.

enum class Verdict {
    Pending,        // never run, or edited since the last run
    Running,        // process is live right now
    Accepted,       // stdout matched expected
    WrongAnswer,    // ran to completion, output differed
    TimeLimit,      // exceeded the time limit and was killed
    RuntimeError,   // non-zero exit or crashed
    CompileError    // the translation unit never built
};

struct TestCase {
    QString input;        // fed to the process on stdin
    QString expected;     // what stdout should contain
    QString actual;       // what stdout actually contained (filled by Judge)
    QString errorText;    // stderr, or the compiler diagnostic on CompileError
    Verdict verdict = Verdict::Pending;
    int elapsedMs = 0;
    int exitCode = 0;
};

// Display helpers — shared by the card badges and the summary line so a
// verdict is never spelled two different ways in two different places.
inline QString verdictLabel(Verdict v) {
    switch (v) {
    case Verdict::Pending:      return QStringLiteral("—");
    case Verdict::Running:      return QStringLiteral("RUN");
    case Verdict::Accepted:     return QStringLiteral("AC");
    case Verdict::WrongAnswer:  return QStringLiteral("WA");
    case Verdict::TimeLimit:    return QStringLiteral("TLE");
    case Verdict::RuntimeError: return QStringLiteral("RE");
    case Verdict::CompileError: return QStringLiteral("CE");
    }
    return QStringLiteral("—");
}

inline QString verdictTooltip(Verdict v) {
    switch (v) {
    case Verdict::Pending:      return QStringLiteral("Not run yet");
    case Verdict::Running:      return QStringLiteral("Running…");
    case Verdict::Accepted:     return QStringLiteral("Accepted");
    case Verdict::WrongAnswer:  return QStringLiteral("Wrong answer");
    case Verdict::TimeLimit:    return QStringLiteral("Time limit exceeded");
    case Verdict::RuntimeError: return QStringLiteral("Runtime error");
    case Verdict::CompileError: return QStringLiteral("Compilation failed");
    }
    return QString();
}
