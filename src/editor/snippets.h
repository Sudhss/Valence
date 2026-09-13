#pragma once
#include <QString>
#include <QVector>

// A code template offered as you type its trigger.
//
// `body` carries a single CARET marker saying where the caret lands after the
// expansion. Using a sentinel rather than a row/column pair means the template
// can be edited freely without anyone recounting lines.
struct Snippet {
    QString trigger;   // what you type
    QString detail;    // the one-line description shown beside it
    QString body;      // insertion text, containing exactly one CARET
};

namespace Snippets {

// U+0001 — cannot occur in source the user typed, so it is unambiguous.
inline const QChar Caret = QChar(0x0001);

inline const QVector<Snippet>& all() {
    static const QVector<Snippet> table = {
        {
            QStringLiteral("cppmain"),
            QStringLiteral("C++ template"),
            QStringLiteral("#include <bits/stdc++.h>\n"
                           "using namespace std;\n"
                           "\n"
                           "int main() {\n"
                           "    \x01\n"
                           "    return 0;\n"
                           "}")
        },
    };
    return table;
}

// Typing fewer characters than this never opens the suggestion list, so normal
// typing is not interrupted by a popup after one or two letters.
inline constexpr int MinPrefix = 3;

inline QVector<Snippet> matching(const QString& prefix) {
    QVector<Snippet> hits;
    if (prefix.size() < MinPrefix) return hits;
    for (const Snippet& s : all()) {
        // Case-insensitive so "CPP" finds it, but the trigger still has to be a
        // genuine prefix — this is a trigger list, not a fuzzy search.
        if (s.trigger.startsWith(prefix, Qt::CaseInsensitive)) hits.append(s);
    }
    return hits;
}

} // namespace Snippets
