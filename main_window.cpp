#include "main_window.h"
#include "editor_view.h"

MainWindow::MainWindow() {
    EditorView* editor = new EditorView(this);
    setCentralWidget(editor);

    setWindowTitle("Valence");
    resize(1000, 700);
}
