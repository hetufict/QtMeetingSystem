#ifndef USERMENU_H
#define USERMENU_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include <QMap>
#include "messagepackage.h"
#include "createmeetingdialog.h"
#include "chatmenu.h"
#include <QListWidget>
#include <QVBoxLayout>
#include <QThread>
#include "meetingroom.h"
#include "filehelper.h"
namespace Ui {
class UserMenu;
}

class UserMenu : public QMainWindow
{
    Q_OBJECT

public:
    explicit UserMenu(QWidget *parent = nullptr,QTcpSocket* socket=nullptr,QString username="");
    ~UserMenu();
    Ui::UserMenu *ui;
    void updateUsrListRequest();//更新用户列表请求
    QString getUsername();//对外接口，返回当前用户名
    void requestMeetingMembers();//更新会议成员列表请求
signals:
    void userMenuClosed(); // 当 UserMenu 关闭时发出的信号
    void startFileSendSignal();
public slots:
    //更新用户列表
    void updateUserList(const MessagePackage& pack);
    //接受私聊消息包
    void onReceivePrivateChat(const MessagePackage& pack);
    //获取私聊对象和消息并发送信号包
    void getPrivateMsg(const QString& objName,const QString &msg);
    //处理新建群聊返回信;
    void onAddaddGroupRet(const MessagePackage& pack);
    void onFlushGroupMembers(const QString& objname);
    //获取群聊对象和消息并发送信号包
    void getGroupMsg(const QString& objName,const QString &msg);
    //接受群聊消息包
    void onReceiveGroupChat(const MessagePackage& pack);
    //获得群成员列表
    void onReceiveGroupMembers(const MessagePackage& pack);
    //接受聊天框文件信息
    void onFileSent(const QString& objname, const QString &filePath,bool group);
    void onsendFileRespond(const MessagePackage& pack);//发送文件回复
    void onrecvPrivateFile(const MessagePackage& pack);//收到发送文件
    void onFlushFileList(const QString& objname,bool group);
    void onReceiveFileList(const MessagePackage& pack);//收到文件列表
    void onRequestFile(const QString& filename,const QString& senderName, const QString &objName,bool group);//请求文件
    void onReceiveFile(const MessagePackage& pack);//下载文件回复包
    void onCreateMeeting(const MessagePackage& pack);//新建会议结果
    void oninvited(const QString &name, const QString &meetName,int& meetingID);//会议邀请
    void onInviteMeetingRespond(const MessagePackage& pack);//邀请结果
    void onGetMeetingInvitation(const MessagePackage& pack);//收到会议邀请
    void onJoinMeeting(const MessagePackage& pack);//收到会议邀请
    void onMeetingRoomClose();
    //void onCloseMeeting(const MessagePackage& pack);//关闭会议处理
    void onMeetingClosed(const MessagePackage& pack);//会议结束处理
    void onMeetingExit(const MessagePackage& pack);//会议结束处理
    void onMeetingMembersList(const MessagePackage& pack);//会议成员列表
protected:
    virtual void  closeEvent(QCloseEvent *event)override;
private slots:
    void list_UserList_itemDoubleClicked(QListWidgetItem *item);

    void on_pb_refllushList_clicked();

    void on_pb_newgroup_clicked();

    void on_pb_invite_clicked();

    void on_list_groupList_itemDoubleClicked(QListWidgetItem *item);

    void on_pb_addmeet_clicked();

    void on_pb_joinmeet_clicked();

private:
    ChatMenu* addNewCharMenu(bool group);
    void addMeetingList(const QString& meetingName,const int& meetingID);
    QTcpSocket* socket;
    QString username;
    QMap<QString,ChatMenu*> chatList;
    ChatMenu* curMenu;
    QVBoxLayout* frameLayout;
    MeetingRoom* mr_w;
    QTimer* timer;//用于定时发送会议成员列表请求
    QThread* fileThrd=nullptr;
    FileHelper* filehelper=nullptr;
};

#endif // USERMENU_H
