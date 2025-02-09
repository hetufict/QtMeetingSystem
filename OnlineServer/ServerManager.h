#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QMap>
#include <QString>
#include <QList>
#include "clientsockethandler.h"
#include "messagepackage.h"
class ServerManager : public QObject
{
    Q_OBJECT
public:
    static ServerManager* instance;     //唯一实例的指针
    static ServerManager* getInstance();   //获取实例的接口
    static void removeInstance();      //删除实例的接口
    //QTcp服务器对象
    QTcpServer* server;
    //客户段处理线程Map<处理线程对象指针,登陆用户名>
    QMap<ClientSocketHandler*,QString> clients;
    //QList<ClientSocketHandler*> clients;
signals:

private slots:
    //内部成员信号处理
    void onNewConnection();//新连接请求-处理槽函数
    void recvClientName(ClientSocketHandler* handler,const QString& name);//登陆成功后设置客户端对应的用户
    void onClientOffline(ClientSocketHandler* handler);//处理客户端断开后相应资源
    void onPrivateChat(MessagePackage& pack);
    void onGroupChat(MessagePackage& pack);
    void onPrivateFile(MessagePackage& pack);
    void onInviteMeeting(MessagePackage& pack);
    void onCleanMeeting(MessagePackage& pack);
private:
    explicit ServerManager(QObject *parent = nullptr);//构造函数
    ~ServerManager() override;      //析构函数私有化
    QHostAddress listenIP;
    int listenPort;
};

#endif // SERVERMANAGER_H
