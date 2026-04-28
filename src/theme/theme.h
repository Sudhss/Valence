#pragma once
#include <QColor>
#include <QFont>
#include <QString>

namespace Theme {
    // ── Background Hierarchy ──
    inline const QColor EditorBg(11, 15, 20);            // #0b0f14 — brightest (focus)
    inline const QColor SidebarBg(10, 13, 18);           // #0a0d12 — slightly dim
    inline const QColor TerminalBg(7, 10, 14);           // #070a0e — most muted
    inline const QColor TitlebarBg(13, 17, 23);          // #0d1117
    inline const QColor PanelBg(12, 16, 24);             // #0c1018

    // ── Accent ──
    inline const QColor Accent(0, 255, 156);             // #00ff9c — neon teal
    inline const QColor AccentGlow(0, 255, 156, 77);     // 30% glow
    inline const QColor AccentDim(0, 255, 156, 38);      // 15% subtle

    // ── Text ──
    inline const QColor TextPrimary(230, 237, 243);      // #e6edf3
    inline const QColor TextSecondary(230, 237, 243, 153); // 60%
    inline const QColor TextMuted(230, 237, 243, 89);    // 35%

    // ── Syntax ──
    inline const QColor SynKeyword(0, 229, 255);         // #00e5ff — bright cyan
    inline const QColor SynType(121, 192, 255);          // #79c0ff — soft blue
    inline const QColor SynString(165, 214, 255);        // #a5d6ff — light blue
    inline const QColor SynComment(230, 237, 243, 77);   // dim grey ~30%
    inline const QColor SynNumber(210, 168, 255);        // #d2a8ff — soft purple
    inline const QColor SynPreprocessor(255, 166, 87);   // #ffa657 — orange
    inline const QColor SynFunction(126, 231, 135);      // #7ee787 — green
    inline const QColor SynPunctuation(230, 237, 243, 128); // 50%

    // ── UI ──
    inline const QColor CurrentLine(255, 255, 255, 8);   // 0.03 opacity
    inline const QColor SelectionBg(0, 255, 156, 38);    // 0.15 opacity
    inline const QColor Border(255, 255, 255, 15);       // ~0.06
    inline const QColor BorderMedium(255, 255, 255, 25); // ~0.1
    inline const QColor ScrollThumb(255, 255, 255, 25);
    inline const QColor ScrollThumbHover(255, 255, 255, 51);

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
