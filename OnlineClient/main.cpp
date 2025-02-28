#include <QApplication>
#include "widget.h"
#include "usrmenu.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 注册元类型
    qRegisterMetaType<MessagePackage>("MessagePackage");
    qRegisterMetaType<MessagePackage>("MessagePackage&");

    // 获取当前工作目录
    QString currentDir = QCoreApplication::applicationDirPath();
    Logger& logger = Logger::getInstance();
    logger.setOutputFile(currentDir + "/OnlineClientLog.txt");
    logger.setLogLevel(Logger::Info);
    LOG(Logger::Info,"Client is start working");

    Widget w;
    w.show();
    return a.exec();
}
