#include <QApplication>
#include "ui/main_window.h"
#include "theme/theme.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Valence");
    app.setApplicationVersion("1.0.0");

    // Set default font
    app.setFont(Theme::editorFont());

    MainWindow window;
    window.show();

    return app.exec();
}
