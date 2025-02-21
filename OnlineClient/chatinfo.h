#ifndef CHATINFO_H
#define CHATINFO_H

#include <QWidget>
#include <QListWidget>
#include <QObject>
#include <QVBoxLayout>
#include "chatmenu.h"
#include "messagepackage.h"
#include "filehelper.h"
namespace Ui {
class ChatInfo;
}

class ChatInfo : public QWidget
{
    Q_OBJECT
public:
    explicit ChatInfo(QWidget *parent = nullptr,QTcpSocket* socket=nullptr,QString username="");
    ~ChatInfo();
        Ui::ChatInfo *ui;
    void updateUsrListRequest();//更新用户列表请求
    QString getUsername();//对外接口，返回当前用户名
signals:
    void userMenuClosed(); // 当 UserMenu 关闭时发出的信号
    void startFileSendSignal();
    void cancelFile();
    void pauseFile();
    void updateFileSpeed(double progressPercentage,double speed,QString fileName,bool send);
    void finishedFile(QString fileName,bool send);
    void setCancelFile(QString fileName,bool send);
public slots:
    //更新用户列表
    void updateUserList(const MessagePackage& pack);
    //接受私聊消息包
    void onReceivePrivateChat(const MessagePackage& pack);
    //获取私聊对象和消息并发送信号包
    void getPrivateMsg(const QString& objName,const QString &msg);
    //处理新建群聊返回信;
    void onAddaddGroupRet(const MessagePackage& pack);
    //刷新群员列表
    void onFlushGroupMembers(const QString& objname);
    //获取群聊对象和消息并发送信号包
    void getGroupMsg(const QString& objName,const QString &msg);
    //接受群聊消息包
    void onReceiveGroupChat(const MessagePackage& pack);
    //获得群成员列表
    void onReceiveGroupMembers(const MessagePackage& pack);
    //接受聊天框文件信息，启动文件线程开启文件传输
    void onFileSent(const QString& objname, const QString &filePath,bool group);
    //更新用户和群组列表
    void onUpdateLists(const MessagePackage& pack);
    //文件线程结束处理
    void onFileSendFinished(MessagePackage& pack);
    void onsendFileRespond(const MessagePackage& pack);//发送文件回复
    void onrecvPrivateFile(const MessagePackage& pack);//收到发送文件
    void onFlushFileList(const QString& objname,bool group);//获取文件列表
    void onReceiveFileList(const MessagePackage& pack);//收到文件列表
    void onRequestFile(const QString& filename,const QString& senderName, const QString &objName,bool group);//请求文件
    void onReceiveFile(const MessagePackage& pack);//下载文件回复包
    void onCancelFile();//取消当前文件操作
    void onPauseFile();//暂停当前文件操作
    void onFileSpeed(double progressPercentage,double speed,QString fileName,bool send);
protected:
private slots:
    void list_UserList_itemDoubleClicked(QListWidgetItem *item);

    void on_pb_newgroup_clicked();

    void on_list_groupList_itemDoubleClicked(QListWidgetItem *item);
private:
    //新建聊天窗口
    ChatMenu* addNewChatMenu(bool group);
    //false,group,path,filename,fileSender,objname
    //创建文件传输线程
    void createFilehelper(const bool &send, const bool &group, const QString &filePath,
                          const QString& filename, const QString &sender, const QString &recer,
                          const qint64& fileSize);
    //恢复文件传输
    void recoverFileProcess();
    QTcpSocket* socket;
    QString username;
    QMap<QString,ChatMenu*> chatList;
    ChatMenu* curMenu;
    QVBoxLayout* frameLayout;
    QTimer* timer;//用于定时发送会议成员列表请求
    QThread* fileThrd=nullptr;
    FileHelper* filehelper=nullptr;
private:
};

#endif // CHATINFO_H
