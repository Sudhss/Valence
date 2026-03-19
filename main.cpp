#include <QApplication>
#include "window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window main;
    main.show();
    return app.exec();
}
