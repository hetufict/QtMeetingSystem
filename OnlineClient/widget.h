#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include "messagepackage.h"
#include "usrmenu.h"
#include "meetingroom.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
signals:
    void updateListRespond(const MessagePackage& pack);
    void receivePrivateChat(const MessagePackage& packpack);
    void addGroupRet(const MessagePackage& packpack);
    void receiveGroupChat(const MessagePackage& packpack);
    void receiveGroupMembers(const MessagePackage& packpack);
    void sendFileRespond(const MessagePackage& pack);//发送私聊文件成功
    void recvPrivateFile(const MessagePackage& pack);//收到私聊文件
    void receiveFileList(const MessagePackage& pack);//收到文件列表
    void receiveFile(const MessagePackage& pack);//下载文件
    void createMeeting(const MessagePackage& pack);//新建会议
    void inviteMeetingRespond(const MessagePackage& pack);
    void getMeetingInvitation(const MessagePackage& pack);//收到会议邀请
    void joinMeeting(const MessagePackage& pack);
    void closeMeeting(const MessagePackage& pack);
    void meetingClosed(const MessagePackage& pack);
    void meetingExit(const MessagePackage& pack);
    void meetingMembersList(const MessagePackage& pack);
    void updateLists(const MessagePackage& pack);
private slots:
    void onReadyRead();
    void on_pb_login_clicked();
    void on_pb_register_clicked();
    void onUserMenuClosed();

private:
    void resultHandler(const MessagePackage& pack);
    void loginResultHandler(const MessagePackage& pack);
    void registerResultHandler(const MessagePackage& pack);
    void adminConnectUserMenu();
    Ui::Widget *ui;
    QString username;
    UsrMenu* usrMenu;
    QTcpSocket* socket;
};
#endif // WIDGET_H
