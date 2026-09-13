#include <QApplication>
#include "ui/main_window.h"
#include "theme/theme.h"

// Dialogs are separate top-level windows, so they inherit nothing from the main
// window's stylesheet. Without this every message box, input prompt and file
// dialog would appear in the host platform's light chrome, in the middle of a
// dark application.
static QString dialogStyle() {
    return QString(
        "QDialog, QMessageBox, QInputDialog, QFileDialog {"
        "  background: %1;"
        "  color: %2;"
        "}"
        "QMessageBox QLabel, QInputDialog QLabel, QDialog QLabel {"
        "  color: %2;"
        "  font-size: 13px;"
        "}"
        "QDialogButtonBox QPushButton, QMessageBox QPushButton {"
        "  background: %3;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: %5px;"
        "  padding: 6px 18px;"
        "  min-width: 72px;"
        "  font-size: 11px;"
        "}"
        "QDialogButtonBox QPushButton:hover, QMessageBox QPushButton:hover {"
        "  background: rgba(255, 255, 255, 0.08);"
        "  color: %6;"
        "}"
        "QDialogButtonBox QPushButton:default, QMessageBox QPushButton:default {"
        "  border: 1px solid %7;"
        "}"
        "QLineEdit {"
        "  background: %8;"
        "  color: %6;"
        "  border: 1px solid %4;"
        "  border-radius: %5px;"
        "  padding: 5px 8px;"
        "  selection-background-color: %9;"
        "}"
        "QLineEdit:focus { border: 1px solid %7; }"
    ).arg(Theme::Overlay.name(),                       // %1
          Theme::TextSecondary.name(QColor::HexArgb),  // %2
          Theme::Raised.name(),                        // %3
          Theme::BorderMedium.name(QColor::HexArgb),   // %4
          QString::number(Theme::Radius),              // %5
          Theme::TextPrimary.name(),                   // %6
          Theme::Accent.name(),                        // %7
          Theme::EditorBg.name(),                      // %8
          Theme::SelectionBg.name(QColor::HexArgb));   // %9
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Valence");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Valence");

    // Set default font
    // The UI face, not the code face. Everything that does not explicitly ask
    // for monospace inherits this.
    app.setFont(Theme::uiFont());
    app.setStyleSheet(dialogStyle());

    MainWindow window;
    // The window restores its own geometry, and maximizes itself on first run.
    window.show();

    return app.exec();
}
