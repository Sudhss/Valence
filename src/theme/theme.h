#pragma once
#include <QColor>
#include <QFont>
#include <QString>
#include <QStringList>

namespace Theme {
    // ── Elevation ────────────────────────────────────────────────────────
    //
    // A dark interface reads as flat plastic when every surface sits within a
    // few points of every other one, which is exactly what this palette used to
    // do: five "different" backgrounds spanning ten RGB values, butted together
    // with identical hairlines. Depth in a dark UI comes from a deliberate
    // ramp — the further forward a surface is, the lighter it sits — plus a
    // highlight along the top edge of raised surfaces, as if lit from above.
    //
    // The neutrals carry a slight blue-violet cast rather than being pure grey;
    // a perfectly neutral dark reads as cheap, and the tint gives the teal
    // accent something to sit against.
    inline const QColor Base    ( 11,  12,  16);   // #0b0c10 app floor, terminal
    inline const QColor Chrome  ( 16,  18,  24);   // #101218 sidebar, status, tab strip
    inline const QColor Surface ( 21,  23,  29);   // #15171d the editor itself
    inline const QColor Raised  ( 27,  30,  38);   // #1b1e26 cards, active tab, panels
    inline const QColor Overlay ( 34,  38,  47);   // #22262f menus, popups — floats above all

    // Existing names, remapped onto the ramp so every call site moves at once.
    inline const QColor EditorBg   = Surface;
    inline const QColor SidebarBg  = Chrome;
    inline const QColor TerminalBg = Base;
    inline const QColor TitlebarBg = Chrome;
    inline const QColor PanelBg    = Chrome;

    // ── Accent ──
    //
    // The signature was #00ff9c: fully saturated, red channel at zero. Neon on
    // near-black is the house style of terminal-hacker themes, and it is the
    // loudest thing in the interface by a wide margin. This is the same teal,
    // pulled back from fluorescent to something with a bit of depth — still
    // unmistakably the brand, no longer shouting.
    //
    // To restore the original exactly, set Accent = AccentNeon.
    inline const QColor AccentNeon(  0, 255, 156);         // #00ff9c — the original
    inline const QColor Accent    ( 47, 224, 160);         // #2fe0a0
    inline const QColor AccentHot ( 92, 240, 186);         // #5cf0ba — hover / focus
    inline const QColor AccentGlow( 47, 224, 160,  64);
    inline const QColor AccentDim ( 47, 224, 160,  30);
    inline const QColor AccentWash( 47, 224, 160,  18);    // large fills only
    inline const QColor AccentBlue( 96, 165, 250);         // #60a5fa — secondary
    inline const QColor OnAccent  (  8,  14,  12);         // text drawn ON the accent

    // ── Text ──
    // Four steps, not three: headings, body, labels, and the things that should
    // barely register until you look for them.
    inline const QColor TextPrimary  (233, 236, 241);      // #e9ecf1
    inline const QColor TextSecondary(233, 236, 241, 160);
    inline const QColor TextMuted    (233, 236, 241, 102);
    inline const QColor TextFaint    (233, 236, 241,  58);

    // ── Syntax ──
    inline const QColor SynKeyword(86, 209, 255);          // #56d1ff — vivid sky blue
    inline const QColor SynType(130, 170, 255);            // #82aaff — periwinkle
    inline const QColor SynString(195, 232, 141);          // #c3e88d — lime green
    inline const QColor SynComment(233, 236, 241, 96);     // recedes, but stays readable
    inline const QColor SynNumber(255, 183, 77);           // #ffb74d — warm orange
    inline const QColor SynPreprocessor(199, 146, 234);    // #c792ea — purple
    inline const QColor SynFunction(130, 231, 135);        // #82e787 — bright green
    inline const QColor SynPunctuation(233, 236, 241, 165); // brackets should not disappear

    // ── Verdict / Judge states (CPH panel, diagnostics) ──
    // Desaturated on purpose: these sit next to code all day and must not shout.
    inline const QColor Success(126, 211, 141);           // #7ed38d — calm green (AC)
    inline const QColor SuccessBg(126, 211, 141, 26);
    inline const QColor Failure(240, 113, 120);           // #f07178 — soft coral (WA)
    inline const QColor FailureBg(240, 113, 120, 26);
    inline const QColor Warning(255, 183, 77);            // #ffb74d — amber (TLE)
    inline const QColor WarningBg(255, 183, 77, 26);
    inline const QColor Pending(233, 236, 241, 90);       // idle / not yet run
    inline const QColor PendingBg(255, 255, 255, 12);

    // ── Edges & light ──
    //
    // A single flat hairline everywhere is what makes panels look stuck onto
    // each other. Real edges have direction: a raised surface catches light on
    // its top edge and casts a darker line at the bottom.
    inline const QColor Border       (255, 255, 255,  16);  // divider between panes
    inline const QColor BorderMedium (255, 255, 255,  26);  // control outlines
    inline const QColor BorderStrong (255, 255, 255,  38);  // focused control
    inline const QColor HighlightTop (255, 255, 255,  20);  // top edge of a raised surface
    inline const QColor ShadowSoft   (  0,   0,   0,  70);  // under floating things
    inline const QColor ShadowDeep   (  0,   0,   0, 120);

    // Hover and selection as tokens, so every widget reacts by the same amount.
    inline const QColor HoverWash    (255, 255, 255,  13);
    inline const QColor ActiveWash   (255, 255, 255,  22);

    // ── UI ──
    inline const QColor CurrentLine(255, 255, 255, 11);   // present, still quiet
    inline const QColor SelectionBg(47, 224, 160, 40);
    inline const QColor ScrollThumb(255, 255, 255, 26);
    inline const QColor ScrollThumbHover(255, 255, 255, 52);

    // ── Metrics ──
    // One spacing scale for every widget, so panels line up without eyeballing.
    inline const int SpaceXS = 4;
    inline const int SpaceS  = 8;
    inline const int SpaceM  = 12;
    inline const int SpaceL  = 16;
    inline const int Radius      = 6;                      // buttons, badges, inputs
    inline const int RadiusPanel = 9;                      // cards, popups, menus
    inline const int RowHeight   = 28;                      // tool buttons, list rows
    inline const int HeaderHeight = 36;                     // panel headers

    // A vertical gradient across a chrome surface is the cheapest convincing
    // depth cue there is, and Qt stylesheets support it directly.
    inline QString chromeGradient(const QColor& top, const QColor& bottom) {
        return QString("qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2)")
            .arg(top.name(), bottom.name());
    }

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
