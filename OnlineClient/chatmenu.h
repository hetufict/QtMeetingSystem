#ifndef CHATMENU_H
#define CHATMENU_H

#include <QWidget>
#include "messagepackage.h"
namespace Ui {
class ChatMenu;
}

class ChatMenu : public QWidget
{
    Q_OBJECT

public:
    explicit ChatMenu(QWidget *parent = nullptr,bool group=false);
    //设置聊天窗口名字,和对象名字
    void setObjName(const QString& name);
    //添加聊天信息到聊天窗口
    void addMsgText(const QString& msg);
    void addMsgText(const QString &msg, bool isSelf);
    void addFile(const QString& filename);
    void addGroupMembers(const QStringList& list);
    void addFileList(const QStringList &filelist,const QStringList &senderlist);
    ~ChatMenu();
    Ui::ChatMenu *ui;
public slots:
signals:
    //发送私聊消息
    void sendPrivateChat(const QString& objName,const QString& msg);
    //发送群聊消息
    void sendGroupChat(const QString& objName,const QString& msg);
    //发送文件信息到usermenu窗口
    void fileSent(const QString& objname, const QString &filePath,bool group);
    //刷新群成员
    void flushGrroupMembers(const QString& objname);
    //刷新文件列表
    void flushFileList(const QString& objname,bool group);
    //下载文件请求
    void requestFile(const QString& fileName, const QString& senderName,const QString &objName,bool group);
private slots:
    //点击发送消息
    void on_pb_sendMsg_clicked();
    //点击发送文件
    void on_pb_sendFile_clicked();

    void on_pb_flushgroupmember_clicked();

    void on_pb_fllushFilelist_clicked();

    void on_pb_downloadFile_clicked();

private:
    //聊天对象用户名或群名
    QString objname;
    bool group;//true为群组聊天界面，false为用户聊天界面
};

#endif // CHATMENU_H
