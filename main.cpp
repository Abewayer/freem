#include <QApplication>
#include "window.h"
//Test exe
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Window w;
    w.show();
    return a.exec();
}
