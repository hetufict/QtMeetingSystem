#include "usrmenu.h"
#include "ui_usrmenu.h"
#include "meetiingmenu.h"

UsrMenu::UsrMenu(QWidget *parent,QTcpSocket* socket,QString username)
    : QMainWindow(parent)
    , ui(new Ui::UsrMenu)
    , m_info(new MeetiingMenu(this,socket,username)) // 创建 MeetiingMenu 实例
    , c_info(new ChatInfo(this,socket,username)) // 创建 ChatInfo 实例
    ,socket(socket)
    ,username(username)
{
    ui->setupUi(this);

    // 确保 tab_meetingInfo 有一个布局管理器
    QVBoxLayout *m_layout = new QVBoxLayout(ui->tab_meetingInfo);
    m_layout->addWidget(m_info); // 将 MeetiingMenu 添加到布局中
    ui->tab_meetingInfo->setLayout(m_layout); // 设置布局

    QVBoxLayout *c_layout = new QVBoxLayout(ui->tab_chatInfo);
    c_layout->addWidget(c_info); // 将 MeetiingMenu 添加到布局中
    ui->tab_chatInfo->setLayout(c_layout); // 设置布局
}

UsrMenu::~UsrMenu()
{
    delete ui;
}

MeetiingMenu* UsrMenu::getMeetingObj()
{
    return m_info;
}

ChatInfo* UsrMenu::getChatObj()
{
    return c_info;
}

QString UsrMenu::getUsername()
{
    return username;
}

void UsrMenu::closeEvent(QCloseEvent *event)
{
    emit usrMenuClosed(); // 发出信号
    QMainWindow::closeEvent(event);
}


