#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    // MeetingRoom m("yuyu","yuyu","yuyu",12,1001,1002,1000);
    // m.show();
    return a.exec();
}
