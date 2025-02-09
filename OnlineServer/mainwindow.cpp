#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDir>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ServerManager::getInstance();

    QDir dir("./fileData");
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "无法创建目录：" << dir.path();
            return;
        }
    }
}

MainWindow::~MainWindow()
{
    ServerManager::removeInstance();
    delete ui;
}
