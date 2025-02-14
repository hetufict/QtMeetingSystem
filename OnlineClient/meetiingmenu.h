#ifndef MEETIINGMENU_H
#define MEETIINGMENU_H

#include <QWidget>
#include "messagepackage.h"
#include "meetingroom.h"
namespace Ui {
class MeetiingMenu;
}

class MeetiingMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MeetiingMenu(QWidget *parent = nullptr,QTcpSocket* socket=nullptr,QString username="");
    ~MeetiingMenu();
    Ui::MeetiingMenu *ui;
    void requestMeetingMembers();//更新会议成员列表请求
public slots:
    void onCreateMeeting(const MessagePackage& pack);//新建会议结果
    void oninvited(const QString &name, const QString &meetName,int& meetingID);//会议邀请
    void onInviteMeetingRespond(const MessagePackage& pack);//邀请结果
    void onGetMeetingInvitation(const MessagePackage& pack);//收到会议邀请
    void onJoinMeeting(const MessagePackage& pack);//收到会议邀请
    void onMeetingRoomClose();//关闭会议处理
    void onMeetingClosed(const MessagePackage& pack);//会议结束处理
    void onMeetingExit(const MessagePackage& pack);//会议结束处理
    void onMeetingMembersList(const MessagePackage& pack);//会议成员列表
private slots:
    void on_pb_addmeet_clicked();

    void on_pb_joinmeet_clicked();
private:
    void addMeetingList(const QString& meetingName,const int& meetingID);
    QTcpSocket* socket;
    QString username;
    MeetingRoom* mr_w;
    QTimer* timer;//用于定时发送会议成员列表请求
};

#endif // MEETIINGMENU_H
