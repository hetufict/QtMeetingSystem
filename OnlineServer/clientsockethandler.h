#ifndef CLIENTSOCKETHANDLER_H
#define CLIENTSOCKETHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QHash>
#include <functional>
#include "dbhelper.h"
#include "messagepackage.h"

using MsgHandler=std::function<void(MessagePackage& pack)>;

class ClientSocketHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientSocketHandler(QTcpSocket* socket,QObject *parent = nullptr);
    ~ClientSocketHandler();
    QString getLoginUser()const;
    QTcpSocket* getSocket()const;
signals:
    void setClientName(ClientSocketHandler* handler,const QString& name);//登陆成功后发送给ServerManager记录用户名
    void clientOffline(ClientSocketHandler* handler);//客户端断开连接后发送给ServerManager处理相应退出动作
    void privateMsg(MessagePackage& pack);
    void groupMsg(MessagePackage& pack);
    void privateFile(MessagePackage& pack);
    void inviteMeeting(MessagePackage& pack);//邀请会议成员
    void cleanMeeting(MessagePackage& pack);//清理会议成员
    void updateLists();
    void updateGroupMemberList(const QString& groupName);
    void updateFileList(const QString& receiver);
private slots:
    void onReadyRead();//客户端请求处理
    void onDisConnected();//客户端断开连接处理
private:
    void handler(const MessagePackage& pack);//协议包请求类型判断
    MsgHandler getHandler(QString msgType);
    void loginHandler(MessagePackage& pack);//登陆请求
    void logoutHandler(MessagePackage& pack);//登出请求
    void registerHandler(MessagePackage& pack);//注册请求
    void updateUserListHandler(MessagePackage& pack);//获取用户列表请求
    void privateChatHandler(MessagePackage& Pack);//私聊请求
    void addGroupHandler(MessagePackage& pack);//建立群组请求
    void inviteGroupHandler(MessagePackage& pack);//邀请群成员处理
    void groupChatHandler(MessagePackage& Pack);//群聊请求
    void getGroupMembersHandler(MessagePackage& pack);
    void privateFileHandler(MessagePackage& pack);//私聊文件
    void getFileListHandler(MessagePackage& pack);//请求文件列表
    void getFileHandler(MessagePackage& pack);//请求文件
    void createMeetingHandler(MessagePackage& pack);//创建会议
    void inviteMeetingHandler(MessagePackage& pack);
    void joinMeetingHandler(MessagePackage& pack);
    void closeMeetingHandler(MessagePackage& pack);
    void membersListHandler(MessagePackage& pack);
    void fileDataHandler(MessagePackage& pack);
    void fileDataRequest(MessagePackage& pack);
    QTcpSocket * socket;
    QString username;
    QHash<QString,MsgHandler> msgHanderMap;

    // 缓存文件数据的字典，键为文件路径，值为缓存的文件数据
    QHash<QString, QByteArray> fileBuffers;
    QHash<QString, qint64> fileReadSize;
};
#endif // CLIENTSOCKETHANDLER_H
