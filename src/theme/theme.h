#pragma once
#include <QColor>
#include <QFont>
#include <QString>

namespace Theme {
    // ── Background Hierarchy (softer, warmer darks — Apple-inspired) ──
    inline const QColor EditorBg(22, 22, 28);             // #16161c — warm dark
    inline const QColor SidebarBg(18, 18, 24);            // #121218 — slightly deeper
    inline const QColor TerminalBg(14, 14, 20);           // #0e0e14 — deepest
    inline const QColor TitlebarBg(24, 24, 30);           // #18181e — tab bar area
    inline const QColor PanelBg(20, 20, 26);              // #14141a

    // ── Accent ──
    inline const QColor Accent(0, 255, 156);              // #00ff9c — neon teal (keep this, it's the signature)
    inline const QColor AccentGlow(0, 255, 156, 60);      // softer glow
    inline const QColor AccentDim(0, 255, 156, 30);       // subtle hint
    inline const QColor AccentBlue(88, 166, 255);         // #58a6ff — secondary accent

    // ── Text ──
    inline const QColor TextPrimary(225, 228, 232);       // #e1e4e8 — slightly softer white
    inline const QColor TextSecondary(225, 228, 232, 140); // 55%
    inline const QColor TextMuted(225, 228, 232, 76);     // 30%

    // ── Syntax (vibrant and colorful — each token type clearly distinct) ──
    inline const QColor SynKeyword(86, 209, 255);          // #56d1ff — vivid sky blue
    inline const QColor SynType(130, 170, 255);            // #82aaff — periwinkle
    inline const QColor SynString(195, 232, 141);          // #c3e88d — lime green
    inline const QColor SynComment(225, 228, 232, 60);     // very dim — fades out
    inline const QColor SynNumber(255, 183, 77);           // #ffb74d — warm orange
    inline const QColor SynPreprocessor(199, 146, 234);    // #c792ea — purple
    inline const QColor SynFunction(130, 231, 135);        // #82e787 — bright green
    inline const QColor SynPunctuation(225, 228, 232, 140); // 55% — more visible

    // ── UI ──
    inline const QColor CurrentLine(255, 255, 255, 6);    // even more subtle
    inline const QColor SelectionBg(0, 255, 156, 25);     // softer selection
    inline const QColor Border(255, 255, 255, 10);        // very subtle borders
    inline const QColor BorderMedium(255, 255, 255, 18);
    inline const QColor ScrollThumb(255, 255, 255, 15);
    inline const QColor ScrollThumbHover(255, 255, 255, 35);

    // ── Font Config ──
    inline const int FontSizeEditor = 13;
    inline const int FontSizeSidebar = 12;
    inline const int FontSizeTerminal = 12;
    inline const int FontSizeStatus = 11;

    inline QFont editorFont() {
        QFont f("JetBrains Mono", FontSizeEditor);
        f.setStyleHint(QFont::Monospace);
        f.setFixedPitch(true);
        return f;
    }

    inline QFont sidebarFont() {
        QFont f("JetBrains Mono", FontSizeSidebar);
        f.setStyleHint(QFont::Monospace);
        f.setFixedPitch(true);
        return f;
    }

    inline QFont terminalFont() {
        QFont f("JetBrains Mono", FontSizeTerminal);
        f.setStyleHint(QFont::Monospace);
        f.setFixedPitch(true);
        return f;
    }

    inline QFont statusFont() {
        QFont f("JetBrains Mono", FontSizeStatus);
        f.setStyleHint(QFont::Monospace);
        f.setFixedPitch(true);
        return f;
    }
}
