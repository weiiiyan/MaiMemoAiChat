#include <QGuiApplication>

#include "ui/UiModule.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    UiModule ui;
    if (!ui.start())
        return -1;

    return QGuiApplication::exec();
}
