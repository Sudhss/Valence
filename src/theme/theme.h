#pragma once
#include <QColor>
#include <QFont>
#include <QString>
#include <QStringList>

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
    inline const QColor OnAccent(10, 12, 14);              // text/icons drawn ON the accent

    // ── Text ──
    inline const QColor TextPrimary(225, 228, 232);       // #e1e4e8 — slightly softer white
    inline const QColor TextSecondary(225, 228, 232, 140); // 55%
    inline const QColor TextMuted(225, 228, 232, 76);     // 30%

    // ── Syntax (vibrant and colorful — each token type clearly distinct) ──
    inline const QColor SynKeyword(86, 209, 255);          // #56d1ff — vivid sky blue
    inline const QColor SynType(130, 170, 255);            // #82aaff — periwinkle
    inline const QColor SynString(195, 232, 141);          // #c3e88d — lime green
    inline const QColor SynComment(225, 228, 232, 96);     // recedes, but stays readable
    inline const QColor SynNumber(255, 183, 77);           // #ffb74d — warm orange
    inline const QColor SynPreprocessor(199, 146, 234);    // #c792ea — purple
    inline const QColor SynFunction(130, 231, 135);        // #82e787 — bright green
    inline const QColor SynPunctuation(225, 228, 232, 165); // brackets should not disappear

    // ── Verdict / Judge states (CPH panel, diagnostics) ──
    // Desaturated on purpose: these sit next to code all day and must not shout.
    inline const QColor Success(126, 211, 141);           // #7ed38d — calm green (AC)
    inline const QColor SuccessBg(126, 211, 141, 22);
    inline const QColor Failure(240, 113, 120);           // #f07178 — soft coral (WA)
    inline const QColor FailureBg(240, 113, 120, 22);
    inline const QColor Warning(255, 183, 77);            // #ffb74d — amber (TLE)
    inline const QColor WarningBg(255, 183, 77, 22);
    inline const QColor Pending(225, 228, 232, 90);       // idle / not yet run
    inline const QColor PendingBg(255, 255, 255, 10);

    // ── UI ──
    inline const QColor CurrentLine(255, 255, 255, 11);   // present, still quiet
    inline const QColor SelectionBg(0, 255, 156, 34);     // softer selection
    inline const QColor Border(255, 255, 255, 10);        // very subtle borders
    inline const QColor BorderMedium(255, 255, 255, 18);
    inline const QColor ScrollThumb(255, 255, 255, 15);
    inline const QColor ScrollThumbHover(255, 255, 255, 35);

    // ── Metrics ──
    // One spacing scale for every widget, so panels line up without eyeballing.
    inline const int SpaceXS = 4;
    inline const int SpaceS  = 8;
    inline const int SpaceM  = 12;
    inline const int SpaceL  = 16;
    inline const int Radius  = 6;                          // the single corner radius
    inline const int RowHeight = 26;                       // tool buttons, list rows
    inline const int HeaderHeight = 32;                    // panel headers

    // ── Type ───────────────────────────────────────────────────────────────
    //
    // Two typefaces, used for two different jobs. Mixing them up is the single
    // most common reason a desktop app looks homemade: monospace is for code
    // and terminal output, and for nothing else. Menus, tabs, labels, buttons
    // and the status bar all belong to the platform's UI face.
    //
    // Families are given as an ordered fallback list rather than one name.
    // Asking for a font that is not installed does not fail loudly — Qt
    // silently substitutes whatever it likes, which is how every glyph in this
    // app ended up in a generic fallback face.

    inline QStringList codeFamilies() {
        return {
            QStringLiteral("JetBrains Mono"),   // if the user installs it, it wins
            QStringLiteral("Cascadia Code"),    // ships with Windows Terminal / VS
            QStringLiteral("Cascadia Mono"),
            QStringLiteral("Consolas"),         // always present on Windows
            QStringLiteral("DejaVu Sans Mono"),
            QStringLiteral("monospace"),
        };
    }

    inline QStringList uiFamilies() {
        return {
            QStringLiteral("Segoe UI Variable Text"),   // Windows 11
            QStringLiteral("Segoe UI"),                 // Windows 10
            QStringLiteral("Inter"),
            QStringLiteral("system-ui"),
            QStringLiteral("sans-serif"),
        };
    }

    // Sizes are in logical pixels, not points. Points made the editor render
    // around 17px on a 96dpi display, which is why everything felt oversized;
    // pixels say what is meant and still scale with the device pixel ratio.
    inline const int FontSizeEditor   = 14;
    inline const int FontSizeTerminal = 13;
    inline const int FontSizeUI       = 13;
    inline const int FontSizeSmall    = 12;

    inline QFont codeFont(int pixelSize = FontSizeEditor) {
        QFont f;
        f.setFamilies(codeFamilies());
        f.setStyleHint(QFont::Monospace);
        f.setFixedPitch(true);
        f.setPixelSize(pixelSize);
        return f;
    }

    inline QFont uiFont(int pixelSize = FontSizeUI) {
        QFont f;
        f.setFamilies(uiFamilies());
        f.setStyleHint(QFont::SansSerif);
        f.setPixelSize(pixelSize);
        return f;
    }

    // A CSS family list for the stylesheets that cannot take a QFont.
    inline QString codeFamilyCss() {
        return QStringLiteral("'JetBrains Mono','Cascadia Code','Cascadia Mono','Consolas',monospace");
    }
    inline QString uiFamilyCss() {
        return QStringLiteral("'Segoe UI Variable Text','Segoe UI','Inter',system-ui,sans-serif");
    }

    // ── Icons ──
    //
    // Windows ships a real icon font. Using it beats scattering Unicode glyphs
    // like ▢ and ⌃ through the UI, which land in whatever fallback face happens
    // to carry them and end up at different weights and baselines from each
    // other — one of the clearest signs of an improvised interface.
    inline QFont iconFont(int pixelSize = 12) {
        QFont f;
        f.setFamilies({QStringLiteral("Segoe Fluent Icons"),    // Windows 11
                       QStringLiteral("Segoe MDL2 Assets")});   // Windows 10
        f.setPixelSize(pixelSize);
        return f;
    }

    namespace Icon {
        inline QString glyph(char16_t c) { return QString(QChar(c)); }
        inline const QString Add       = glyph(0xE710);
        inline const QString NewFolder = glyph(0xE8F4);
        inline const QString Refresh   = glyph(0xE72C);
        inline const QString Collapse  = glyph(0xE70E);   // chevron up
        inline const QString Close     = glyph(0xE711);
        inline const QString Play      = glyph(0xE768);
        inline const QString Stop      = glyph(0xE71A);
        inline const QString Delete    = glyph(0xE74D);   // waste basket
        inline const QString Clear     = glyph(0xE894);
        inline const QString Folder    = glyph(0xE8B7);
    }

    // ── Named roles ──
    // Call sites ask for the role, not the face, so the mapping above is the
    // only place that ever has to change.
    inline QFont editorFont()   { return codeFont(FontSizeEditor); }     // the code surface
    inline QFont terminalFont() { return codeFont(FontSizeTerminal); }   // shell + test I/O
    inline QFont sidebarFont()  { return uiFont(FontSizeUI); }           // explorer, tabs
    inline QFont statusFont()   { return uiFont(FontSizeSmall); }        // status bar, headers
}
