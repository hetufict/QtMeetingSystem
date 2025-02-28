#include <QApplication>
#include "mainwindow.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Initialize logger
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(Logger::Debug); // Set log level
    logger.setOutputFile("server_log.txt"); // Set log output file
    LOG(Logger::Info,"Server is start working");

    MainWindow w;
    w.show();
    return a.exec();
}
