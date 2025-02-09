#ifndef CLIENTSOCKETHANDLER_H
#define CLIENTSOCKETHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QThreadPool>
#include <QRunnable>
#include <QHash>
#include "clienttask.h"
#include "messagepackage.h"
struct currentFileDataStream  {
    QString fileName;      // 文件名
    qint64 size;          // 文件总大小
    qint64 readSoFar;     // 已读取的数据大小
    QByteArray fileData;  // 接收到的文件数据
    QString reciever;//接受者
    bool group;
    // 重置结构体，以便开始接收新的文件数据
    void reset() {
        fileName.clear();
        size = 0;
        readSoFar = 0;
        fileData.clear();
        group=false;
    }
};

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
private slots:
    void onReadyRead();//客户端请求处理
    void onDisConnected();//客户端断开连接处理
private:
    void getFile(const MessagePackage& pack);//
    void acceptFile();
    void handler(const MessagePackage& pack);//协议包请求类型判断
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
    void handleReceivedFile(const QByteArray &fileData, const QString &fileName ,const QString &receiver);//处理接受到的文件
    void getFileListHandler(MessagePackage& pack);
    void getFileHandler(MessagePackage& pack);
    void createMeetingHandler(MessagePackage& pack);
    void inviteMeetingHandler(MessagePackage& pack);
    void joinMeetingHandler(MessagePackage& pack);
    void closeMeetingHandler(MessagePackage& pack);
    void membersListHandler(MessagePackage& pack);
    void fileDataHandler(MessagePackage& pack);
    void writeToFile(const QString &path, const QByteArray &data);
    void fileTransferCompleted(const QString &filePath);
    QTcpSocket * socket;
    QString username;
    currentFileDataStream fileReader;
    // 缓存文件数据的字典，键为文件路径，值为缓存的文件数据
    QHash<QString, QByteArray> fileBuffers;
    QHash<QString, qint64> fileReadSize;
};
#endif // CLIENTSOCKETHANDLER_H
