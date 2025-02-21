#include "widget.h"
#include "usrmenu.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 注册元类型
    qRegisterMetaType<MessagePackage>("MessagePackage");
    qRegisterMetaType<MessagePackage>("MessagePackage&");
    Widget w;
    w.show();
    return a.exec();
}
