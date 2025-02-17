#include "ServerManager.h"
#include "ClientSocketHandler.h"
#include <QDebug>
// 初始化静态成员变量
ServerManager* ServerManager::instance = nullptr;

// 获取实例的接口
ServerManager* ServerManager::getInstance()
{
    if (instance == nullptr) {
        instance = new ServerManager();
    }
    return instance;
}

// 删除实例的接口
void ServerManager::removeInstance()
{
    delete instance;
    instance = nullptr;
}

void ServerManager::onUpdateLists() {
    MessagePackage pack;
    QStringList onlineUsers = DBHelper::getInstance()->getOnlineUsers(); // 在线用户
    QStringList offlineUsers = DBHelper::getInstance()->getOfflineUsers(); // 离线用户
    pack.setType(MessagePackage::Key_Type_UpdateLists);
    pack.addValue(MessagePackage::Key_OnlineUsers, onlineUsers);
    pack.addValue(MessagePackage::Key_OfflineUsers, offlineUsers);

    for (QMap<ClientSocketHandler*, QString>::const_iterator it = clients.constBegin(); it != clients.constEnd(); ++it) {
        ClientSocketHandler* handler = it.key(); // 获取键（ClientSocketHandler*）
        QString username = it.value(); // 获取值（QString）
        QStringList mygroups = DBHelper::getInstance()->getGroupNames(username);//用户加入的群组
        pack.addValue(MessagePackage::Key_UserGroupList, mygroups);

        // 发送消息包到客户端
        pack.sendMsg(handler->getSocket());
    }
}

void ServerManager::onUpdateGroupMemberList(const QString& groupName)
{
    //获取所有群组成员
    QStringList groupMembers=DBHelper::getInstance()->getGroupMembers(groupName);
    //遍历群员列表，发送群组成员信息
    for(const auto& it:groupMembers){
        MessagePackage pack;
        pack.setType(MessagePackage::Key_Type_GetGroupMembers);
        pack.addValue(MessagePackage::Key_UserList,groupMembers);
        if(users.contains(it)){
            pack.sendMsg(users[it]->getSocket());
        }
    }
}

void ServerManager::onUpdateFileList(const QString &groupName)
{
    //获取所有群组成员
    QStringList groupMembers=DBHelper::getInstance()->getGroupMembers(groupName);
    QStringList filelist=DBHelper::getInstance()->getFileList(groupName);
    QStringList senderlist=DBHelper::getInstance()->getSenderList(filelist,"",groupName,true);
    qDebug()<<"filelist size:"<<filelist.size()<<" sendlerlist size:"<<senderlist.size();
    //遍历群员列表，发送群组成员信息
    for(const auto& it:groupMembers){
        MessagePackage pack;
        pack.setType(MessagePackage::Key_Type_GetFileList);
        pack.addValue(MessagePackage::key_FileList,filelist);
        pack.addValue(MessagePackage::key_SenderList,senderlist);
        pack.addValue(MessagePackage::Key_Name,groupName);
        if(users.contains(it)){
            pack.sendMsg(users[it]->getSocket());
        }
    }
}
void ServerManager::onNewConnection()
{
    //同意客户端连接，并获取客户端套接字
    QTcpSocket * socket = server->nextPendingConnection(); //同意连接得到套接字的指针
    ClientSocketHandler* handler = new ClientSocketHandler(socket,this);  //生成客户端套接字处理-管理套接字
    //clients.append(handler); //将处理类放到容器中统一管理
    clients[handler]="";
    // 检查套接字是否连接
    if (socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Client connected:" << socket->peerAddress().toString()<<" "<<socket->peerPort();
    } else {
        qDebug() << "Client not connected yet.";
    }
    connect(handler,&ClientSocketHandler::setClientName,this,&ServerManager::recvClientName);
    connect(handler,&ClientSocketHandler::clientOffline,this,&ServerManager::onClientOffline);
    connect(handler,&ClientSocketHandler::privateMsg,this,&ServerManager::onPrivateChat);
    connect(handler,&ClientSocketHandler::groupMsg,this,&ServerManager::onGroupChat);
    connect(handler,&ClientSocketHandler::privateFile,this,&ServerManager::onPrivateFile);
    connect(handler,&ClientSocketHandler::inviteMeeting,this,&ServerManager::onInviteMeeting);
    connect(handler,&ClientSocketHandler::cleanMeeting,this,&ServerManager::onCleanMeeting);
    connect(handler,&ClientSocketHandler::updateLists,this,&ServerManager::onUpdateLists);
    connect(handler,&ClientSocketHandler::updateGroupMemberList,this,&ServerManager::onUpdateGroupMemberList);
    connect(handler,&ClientSocketHandler::updateFileList,this,&ServerManager::onUpdateFileList);
}

void ServerManager::recvClientName(ClientSocketHandler* handler,const QString& name)
{
    clients[handler]=name;
    users[name]=handler;
    //qDebug()<<"set handler name :"<<clients[handler];
}

void ServerManager::onClientOffline(ClientSocketHandler *handler)
{
    //将handler和user移出map容器
    auto it=clients.constFind(handler);
    auto user=users.constFind(it.value());
    if(user!=users.constEnd()){
        users.erase(user);
    }
    if(it!=clients.constEnd())
    {
        clients.erase(it);
    }
    //删除handler
    if (handler) {
        delete const_cast<ClientSocketHandler*>(handler); // 慎用 const_cast
        handler = nullptr; // 防止重复删除
    }
}

void ServerManager::onPrivateChat(MessagePackage &pack)
{
    QString recver=pack.getStringValue(MessagePackage::Key_Receiver);
    //qDebug() << "send to reading" << recver;
    for(auto handler=clients.begin();handler!=clients.end();handler++)
    {
        if(handler.key()->getLoginUser()==recver)
        {
            //qDebug() << "send to" << recver;
            pack.sendMsg(handler.key()->getSocket());
            break;
        }
    }
}

void ServerManager::onGroupChat(MessagePackage &pack) {
    QString groupName = pack.getStringValue(MessagePackage::Key_Receiver);
    QString sender = pack.getStringValue(MessagePackage::Key_Sender);
    QStringList members = DBHelper::getInstance()->getGroupMembers(groupName);
    //qDebug() << "send to" <<groupName<<"members";
    // 遍历所有群成员
    for (const QString &memberName : members) {
        if (memberName == sender) {
            // 跳过消息的发送者
            continue;
        }
        for(auto handler=clients.begin();handler!=clients.end();handler++){
            if (handler.key()->getLoginUser()== memberName) {
                pack.sendMsg(handler.key()->getSocket());
                //qDebug() << "send to" <<groupName<<"members:"<<memberName;
            }
        }
    }
}

void ServerManager::onPrivateFile(MessagePackage &pack)
{
    QString recver=pack.getStringValue(MessagePackage::Key_Receiver);
    //qDebug() << "send to reading" << recver;
    pack.sendMsg(users[recver]->getSocket());
    // for(auto handler=clients.begin();handler!=clients.end();handler++)
    // {
    //     if(handler.key()->getLoginUser()==recver)
    //     {
    //         qDebug() << "send fileinfo to" << recver;
    //         pack.sendMsg(handler.key()->getSocket());
    //         break;
    //     }
    // }
}

void ServerManager::onInviteMeeting(MessagePackage &pack)
{
    QString recver=pack.getStringValue(MessagePackage::Key_Receiver);
    //qDebug() << "send to reading" << recver;
    for(auto handler=clients.begin();handler!=clients.end();handler++)
    {
        if(handler.key()->getLoginUser()==recver)
        {
            //qDebug() << "send to" << recver;
            pack.sendMsg(handler.key()->getSocket());
            break;
        }
    }
}

void ServerManager::onCleanMeeting(MessagePackage &pack)
{
    //QString groupName = pack.getStringValue(MessagePackage::Key_Receiver);
    //QString sender = pack.getStringValue(MessagePackage::Key_Sender);
    QString hostname=pack.getStringValue(MessagePackage::Key_Name);
    QStringList members = pack.getListValue(MessagePackage::Key_UserList);
    //qDebug() << "send to" <<groupName<<"members";
    // 遍历所有群成员
    for (const QString &memberName : members) {
        if (memberName == hostname) {
            // 跳过消息的发送者
            continue;
        }
        for(auto handler=clients.begin();handler!=clients.end();handler++){
            if (handler.key()->getLoginUser()== memberName) {
                pack.sendMsg(handler.key()->getSocket());
                //qDebug() << "send to" <<groupName<<"members:"<<memberName;
            }
        }
    }
}

// 构造函数
ServerManager::ServerManager(QObject *parent) : QObject(parent), listenIP(QHostAddress("127.0.0.1")), listenPort(8888)
{
    //初始化连接:启动服务器开始监听
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &ServerManager::onNewConnection);
    //server->listen(listenIP, listenPort);
    if (!server->listen(listenIP, listenPort)) {
        // 处理监听失败的情况
        qDebug() << "Server failed to start!";
    } else {
        qDebug() << "Server started on port" << listenPort;
    }
}

ServerManager::~ServerManager() {
    // 清理资源
    for (auto it = clients.begin(); it != clients.end();) {
        // 获取 handler 指针
        ClientSocketHandler* handler = it.key();
        if (handler != nullptr) {
            // 下线用户
            QString username = it.value();
            if (!username.isEmpty()) {
                // 确保 DBHelper 实例存在且方法正确执行
                if (DBHelper::getInstance() && DBHelper::getInstance()->userOffline(username)) {
                    qDebug() << "用户下线：" << username;
                } else {
                    qDebug() << "用户下线失败：" << username;
                }
            }
        }
        // 从 QMap 中移除当前项并移动到下一个项
        it = clients.erase(it); // 使用 erase 返回的迭代器
    }
    server->close();
}
