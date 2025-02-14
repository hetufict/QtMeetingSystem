#ifndef USRMENU_H
#define USRMENU_H

#include <QMainWindow>
#include "meetiingmenu.h"
#include "chatinfo.h"
namespace Ui {
class UsrMenu;
}

class UsrMenu : public QMainWindow
{
    Q_OBJECT

public:
    explicit UsrMenu(QWidget *parent = nullptr,QTcpSocket* socket=nullptr,QString username="");
    ~UsrMenu();
    MeetiingMenu* getMeetingObj();
    ChatInfo* getChatObj();
    QString getUsername();
signals:
    void usrMenuClosed();
protected:
    virtual void  closeEvent(QCloseEvent *event)override;
private:
    Ui::UsrMenu *ui;
    MeetiingMenu* m_info;
    ChatInfo* c_info;
    QTcpSocket* socket;
    QString username;
    QTimer* timer;//用于定时发送会议成员列表请求
};

#endif // USRMENU_H
