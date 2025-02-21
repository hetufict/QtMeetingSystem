#include "clientsockethandler.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
ClientSocketHandler::ClientSocketHandler(QTcpSocket *socket, QObject *parent) :
    QObject(parent),socket(socket),username("")
{
    if (!socket) {
        qWarning() << "ClientSocketHandler constructed with a null socket.";
        return;
    }
    /********信号与槽函数******/
    //接收数据
    connect(socket,&QTcpSocket::readyRead,this,&ClientSocketHandler::onReadyRead);
    //断开连接处理
    connect(socket,&QTcpSocket::disconnected,this,&ClientSocketHandler::onDisConnected);

    /********绑定处理函数******/
    //登录
    msgHanderMap.insert(MessagePackage::Key_Type_Login,std::bind(&ClientSocketHandler::loginHandler,this,std::placeholders::_1));
    //登出
    msgHanderMap.insert(MessagePackage::Key_Type_Logout,std::bind(&ClientSocketHandler::logoutHandler,this,std::placeholders::_1));
    //注册
    msgHanderMap.insert(MessagePackage::Key_Type_Register,std::bind(&ClientSocketHandler::registerHandler,this,std::placeholders::_1));
    //获取用户列表
    msgHanderMap.insert(MessagePackage::Key_Type_UpdateList,std::bind(&ClientSocketHandler::updateUserListHandler,this,std::placeholders::_1));
    //私聊
    msgHanderMap.insert(MessagePackage::Key_Type_PrivateChat,std::bind(&ClientSocketHandler::privateChatHandler,this,std::placeholders::_1));
    //添加群组
    msgHanderMap.insert(MessagePackage::Key_Type_AddGroup,std::bind(&ClientSocketHandler::addGroupHandler,this,std::placeholders::_1));
    //邀请群员
    msgHanderMap.insert(MessagePackage::Key_Type_InviteGroupMember,std::bind(&ClientSocketHandler::inviteGroupHandler,this,std::placeholders::_1));
    //群聊
    msgHanderMap.insert(MessagePackage::Key_Type_GroupChat,std::bind(&ClientSocketHandler::groupChatHandler,this,std::placeholders::_1));
    //获取群员列表
    msgHanderMap.insert(MessagePackage::Key_Type_GetGroupMembers,std::bind(&ClientSocketHandler::getGroupMembersHandler,this,std::placeholders::_1));
    //私聊文件
    msgHanderMap.insert(MessagePackage::Key_Type_PrivateFile,std::bind(&ClientSocketHandler::privateFileHandler,this,std::placeholders::_1));
    //获取文件列表
    msgHanderMap.insert(MessagePackage::Key_Type_GetFileList,std::bind(&ClientSocketHandler::getFileListHandler,this,std::placeholders::_1));
    //文件
    msgHanderMap.insert(MessagePackage::Key_Type_GetFile,std::bind(&ClientSocketHandler::getFileHandler,this,std::placeholders::_1));
    //创建会议
    msgHanderMap.insert(MessagePackage::Key_Type_CreateMeeting,std::bind(&ClientSocketHandler::createMeetingHandler,this,std::placeholders::_1));
    //邀请入会
    msgHanderMap.insert(MessagePackage::Key_Type_InviteMeeting,std::bind(&ClientSocketHandler::inviteMeetingHandler,this,std::placeholders::_1));
    //参加会议
    msgHanderMap.insert(MessagePackage::Key_Type_JoinMeeting,std::bind(&ClientSocketHandler::joinMeetingHandler,this,std::placeholders::_1));
    //关闭会议
    msgHanderMap.insert(MessagePackage::Key_Type_CloseMeeting,std::bind(&ClientSocketHandler::closeMeetingHandler,this,std::placeholders::_1));
    //会议成员列表
    msgHanderMap.insert(MessagePackage::Key_Type_MembersList,std::bind(&ClientSocketHandler::membersListHandler,this,std::placeholders::_1));
    //文件数据
    msgHanderMap.insert(MessagePackage::Key_Type_FILEDATA,std::bind(&ClientSocketHandler::fileDataHandler,this,std::placeholders::_1));
    //文件下载
    msgHanderMap.insert(MessagePackage::Key_Type_FileDataRequest,std::bind(&ClientSocketHandler::fileDataRequest,this,std::placeholders::_1));
    //文件取消上传
    msgHanderMap.insert(MessagePackage::Key_Type_CancelUpload,std::bind(&ClientSocketHandler::fileCancelUpload,this,std::placeholders::_1));
}

ClientSocketHandler::~ClientSocketHandler()
{
    //服务器关闭，delete其管理的handler对象时，确保用户全部正常退出
    if(DBHelper::getInstance()->userIsOnline(username))
    {
        DBHelper::getInstance()->userOffline(username);
    }
    //关闭套接字
    if(socket)
    {
        socket->close();
    }
}

QString ClientSocketHandler::getLoginUser()const
{
    return username;
}

QTcpSocket *ClientSocketHandler::getSocket()const
{
    return socket;
}

void ClientSocketHandler::onReadyRead() {
    while (socket->bytesAvailable() > 0) {
        // 读取协议包
        MessagePackage pack(nullptr, 0);
        pack.recvMsg(socket);
        // 处理协议包
        handler(pack);
    }
}


void ClientSocketHandler::getFileListHandler(MessagePackage &pack)
{
    QString sender=pack.getStringValue(MessagePackage::Key_Sender);
    QString objname=pack.getStringValue(MessagePackage::Key_Name);
    qDebug()<<"??"<<objname;
    int group=pack.getIntValue(MessagePackage::Key_Result);
    QStringList filelist=DBHelper::getInstance()->getFileList(sender,objname,group);
    QStringList senderList=DBHelper::getInstance()->getSenderList(filelist,sender,objname,group);
    pack.addValue(MessagePackage::key_FileList,filelist);
    pack.addValue(MessagePackage::key_SenderList,senderList);
    pack.sendMsg(socket);
}

//处理文件资源请求，回应文件资源是否存在
void ClientSocketHandler::getFileHandler(MessagePackage &pack)
{
    //解析包
    QString packSender=pack.getStringValue(MessagePackage::Key_Sender);
    QString objname=pack.getStringValue(MessagePackage::Key_Receiver);
    QString fileSender=pack.getStringValue(MessagePackage::Key_Name);
    QString filename=pack.getStringValue(MessagePackage::key_FileName);
    int group=pack.getIntValue(MessagePackage::Key_Result);
    QString fileName=group==1?"group":"private";
    QString path="./fileData/";
    if(group==1){
        fileName+=fileSender+objname+filename;//文件发送者+聊天对象（群聊名称）+文件名
    }else{
        if(packSender==fileSender){//消息发送者发送的文件
            fileName+=packSender+objname+filename;//文件发送者+聊天对象+文件名
        }
        else{//对方发送的文件
            fileName+=fileSender+packSender+filename;//文件发送者+自己+文件名
        }
    }
    path+=fileName;
    QFile file(path);
    qDebug()<<"sender"<<packSender<<"objname:"<<objname<<"fileSender:"<<fileSender;
    qDebug()<<"filePath::"<<path;
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << file.fileName();
        return;
    }
    qint64 fileSize = file.size();
    QString rfileName = QFileInfo(fileName).fileName();
    qDebug()<<"file:"<<rfileName<<"size:"<<fileSize;
    // 准备协议包
    pack.addValue(MessagePackage::key_FileSize, fileSize); // 发送的文件大小
    // 发送协议包
    pack.sendMsg(socket);
    file.close();
    qDebug()<<"文件关闭："<<rfileName;
}

void ClientSocketHandler::createMeetingHandler(MessagePackage &pack)
{
    QString meetingName=pack.getStringValue(MessagePackage::key_MeetingName);
    QString hostname=pack.getStringValue(MessagePackage::Key_Name);
    int i=0;
    for(;i<3000;++i)
    {
        if(!DBHelper::getInstance()->ports[i]){
            break;
        }
    }
    if(i==3000){
        pack.addValue(MessagePackage::Key_Result,0);
        pack.sendMsg(socket);
        return;
    }
    int port1=1000+i,port2=1001+i,port3=1002+i;
    bool joined=false;
    int ret=DBHelper::getInstance()->addMeeting(meetingName,hostname,port1,port2,port3);
    if(ret!=-1){
        DBHelper::getInstance()->ports[i]=true;
        DBHelper::getInstance()->ports[i+1]=true;
        DBHelper::getInstance()->ports[i+2]=true;

        pack.addValue(MessagePackage::Key_MessagePort,port1);
        pack.addValue(MessagePackage::Key_VideoPort,port2);
        pack.addValue(MessagePackage::Key_MediaPort,port3);
        DBHelper::getInstance()->inviteMeeting(ret,meetingName,hostname,true);
    }
    pack.addValue(MessagePackage::key_MeetingID,ret);
    pack.sendMsg(socket);
}

void ClientSocketHandler::inviteMeetingHandler(MessagePackage &pack)
{
    QString name=pack.getStringValue(MessagePackage::Key_Receiver);
    QString meetingName=pack.getStringValue(MessagePackage::key_MeetingName);
    int meetingID=pack.getIntValue(MessagePackage::key_MeetingID);
    bool ret=DBHelper::getInstance()->userIsOnline(name);//检查用户是否在线
    qDebug()<<"online:"<<ret;
    if(ret)
    {
        //检查用户是否有记录
        ret=DBHelper::getInstance()->isMemberExists(meetingID,meetingName,name);
        qDebug()<<"exits:"<<ret;
        if(!ret){//不在则发送邀请
            ret=DBHelper::getInstance()->inviteMeeting(meetingID,meetingName,name);
            emit inviteMeeting(pack);
        }
    }
    pack.addValue(MessagePackage::Key_Result,ret);
    pack.sendMsg(socket);
}

void ClientSocketHandler::joinMeetingHandler(MessagePackage &pack)
{
    QString name=pack.getStringValue(MessagePackage::Key_Sender);
    QString meetingName=pack.getStringValue(MessagePackage::key_MeetingName);
    int meetingID=pack.getIntValue(MessagePackage::key_MeetingID);
    qDebug()<<"meetingName:"<<meetingName<<"meetingID:"<<meetingID;
    //检查用户在不在会议中
    bool ret=DBHelper::getInstance()->isMemberJoined(meetingID,meetingName,name);
    //qDebug()<<"name"<<name<<"joined:"<<ret;
    if(ret){
        ret=DBHelper::getInstance()->joinMeeting(meetingID,meetingName,name);
        //qDebug()<<"name"<<name<<"join:"<<ret;
        int* ports=DBHelper::getInstance()->getMeetingPorts(meetingID,meetingName);
        if(ports!=nullptr){
            pack.addValue(MessagePackage::Key_MessagePort,ports[0]);
            pack.addValue(MessagePackage::Key_VideoPort,ports[1]);
            pack.addValue(MessagePackage::Key_MediaPort,ports[2]);
            delete[] ports;
            ports=nullptr;
        }else{
            ret=false;
        }
    }
    pack.addValue(MessagePackage::Key_Result,ret);
    pack.sendMsg(socket);
}

void ClientSocketHandler::closeMeetingHandler(MessagePackage &pack)
{
    int meetingID=pack.getIntValue(MessagePackage::key_MeetingID);
    QString meetingName=pack.getStringValue(MessagePackage::key_MeetingName);
    QString user=pack.getStringValue(MessagePackage::Key_Sender);
    QString hostname=pack.getStringValue(MessagePackage::Key_Name);
    bool ret=false;
    if(hostname==user){//主持人将会议关闭
        //关闭会议（设置会议表信息状态，恢复端口号）
        ret=DBHelper::getInstance()->closeMeeting(meetingID,meetingName);
        //通知还在会议中的参会者
        //获取参会成员列表
        QStringList members=DBHelper::getInstance()->getMeetingMembers(meetingID,meetingName);
        //改变成员列表中人员的参会状态
        DBHelper::getInstance()->clearMeetingRoom(meetingID,meetingName,members);
        //通知对应用户
        pack.addValue(MessagePackage::Key_CloseMeeting,2);
        pack.addValue(MessagePackage::Key_Type_CleanMeeting,1);
        pack.addValue(MessagePackage::Key_UserList,members);
        pack.addValue(MessagePackage::Key_Result,ret);
        emit cleanMeeting(pack);
    }else{//非主持人用户离开会议
        qDebug()<<"meeting ID:"<<meetingID<<"meetingName:"<<meetingName<<"member:"<<user;
        int leave=DBHelper::getInstance()->leaveMeeting(meetingID,meetingName,user);//离开会议(改变表中参会状态)
        pack.addValue(MessagePackage::Key_CloseMeeting,leave);
        //qDebug()<<leave<<"1111";
    }
    pack.sendMsg(socket);
}

void ClientSocketHandler::membersListHandler(MessagePackage &pack)
{
    int meetingID=pack.getIntValue(MessagePackage::key_MeetingID);
    QString meetingName=pack.getStringValue(MessagePackage::key_MeetingName);
    QStringList membersIn=DBHelper::getInstance()->getInMembers(meetingID,meetingName);
    QStringList membersAbsent=DBHelper::getInstance()->getAbsentMembers(meetingID,meetingName);
    QStringList membersLeft=DBHelper::getInstance()->getLeaveMembers(meetingID,meetingName);
    pack.addValue(MessagePackage::Key_MeetingMembersIn,membersIn);
    pack.addValue(MessagePackage::Key_MeetingMembersAbsent,membersAbsent);
    pack.addValue(MessagePackage::Key_MeetingMembersLeave,membersLeft);
    pack.sendMsg(socket);
}

const size_t WRITE_THRESHOLD=1024*1024;

void ClientSocketHandler::fileDataHandler(MessagePackage &pack)
{
    qDebug()<<"read file size:"<<pack.getIntValue(MessagePackage::Key_SentSize);
    //设置文件存储路径
    QString path="./fileData/";
    QString filePath;
    int group=pack.getIntValue(MessagePackage::Key_Result)==1;
    if(group){
        filePath="group";
    }
    else{
        filePath="private";
    }
    QString sender=pack.getStringValue(MessagePackage::Key_Sender);
    QString receiver=pack.getStringValue(MessagePackage::Key_Receiver);
    QString fileName=pack.getStringValue(MessagePackage::key_FileName);
    QByteArray fileData=pack.getFileValue(MessagePackage::key_FileData);
    qint64 filesize=pack.getIntValue(MessagePackage::key_FileSize);
    int sentSize=pack.getIntValue(MessagePackage::Key_SentSize);
    filePath+=sender+receiver+fileName;
    path+=filePath;
    qDebug() << "文件存储路径：" << path;
    // 写入数据
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "无法打开文件" << path << "错误：" << file.errorString();
        return;
    }
    //qDebug()<<"buffer contain:\n"<<fileData.toStdString();
    file.seek(sentSize-fileData.size()); // 定位到文件的上次写入的位置
    file.write(fileData);   // 写入数据
    file.close();           // 关闭文件
    // 记录文件已接受大小，回复接受数据确认
    if(sentSize < filesize){
        MessagePackage respack;
        respack.setType(MessagePackage::Key_Type_FilePos); // 设置发送包类型
        respack.addValue(MessagePackage::Key_Sender, sender); // 发送者
        respack.addValue(MessagePackage::Key_Receiver, receiver); // 发送对象
        respack.addValue(MessagePackage::key_FileName, fileName); // 发送的文件名
        respack.addValue(MessagePackage::key_FileSize,sentSize);//已经发送的大小
        respack.sendMsg(socket); // 回发送消息
    }
    else if (sentSize== filesize){//如果文件上传完成
        if(group==0){//加入数据库中
            QString filePath="private"+sender+receiver+fileName;
            path+=filePath;
            DBHelper::getInstance()->addFile(path,fileName,"private",sender,receiver,filesize);
        }
        else{
            QString filePath="group"+sender+receiver+fileName;
            path+=filePath;
            DBHelper::getInstance()->addFile(path,fileName,"group",sender,receiver,filesize);
        }
        //设置回发数据包，(发送者上传成功应答，接受者获得文件信息)
        MessagePackage respack;
        respack.setType(MessagePackage::Key_Type_FileOK); // 设置发送包类型
        QStringList filelist=DBHelper::getInstance()->getFileList(sender,receiver,group);
        QStringList senderList=DBHelper::getInstance()->getSenderList(filelist,sender,receiver,group);
        respack.addValue(MessagePackage::key_FileList,filelist);
        respack.addValue(MessagePackage::key_SenderList,senderList);
        respack.addValue(MessagePackage::Key_Sender,sender); // 发送者
        respack.addValue(MessagePackage::Key_Receiver,receiver); // 发送对象
        respack.addValue(MessagePackage::key_FileName,fileName); // 发送的文件名
        respack.addValue(MessagePackage::Key_Result,group); // 区分群组
        respack.sendMsg(socket); // 发送结果给当前客户端 退出文件线程
        if(group!=1){//发送结果给私聊双方
            emit privateFile(respack);
        }
        else{
            emit updateFileList(receiver);//发送结果给群组成员
        }

    }
}

//发送文件数据给客户端
void ClientSocketHandler::fileDataRequest(MessagePackage &pack) {
    // 找到文件存储路径
    QString sender = pack.getStringValue(MessagePackage::Key_Sender);
    QString receiver = pack.getStringValue(MessagePackage::Key_Receiver);
    QString fileName = pack.getStringValue(MessagePackage::key_FileName);
    qint64 fileSize = pack.getIntValue(MessagePackage::key_FileSize);
    int sentSize = pack.getIntValue(MessagePackage::Key_SentSize);
    QString path = "./fileData/";
    QString filePath;
    filePath = (pack.getIntValue(MessagePackage::Key_Result) == 1) ? "group" : "private";
    filePath += sender + receiver + fileName;
    path += filePath;
    qDebug() << "文件存储路径：" << path;

    // 打开文件
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件" << path << "错误：" << file.errorString();
        return;
    }
    // 检查文件大小
    qint64 fileSizeActual = file.size();
    if (fileSizeActual < fileSize) {
        qDebug() << "文件大小不匹配，文件大小：" << fileSizeActual << "，期望大小：" << fileSize;
        file.close();
        return;
    }
    // 检查套接字状态
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Socket error:" << socket->error();
        qDebug() << "Socket error string:" << socket->errorString();
        file.close();
        return;
    }
    // 定位到当前文件指针位置
    if (!file.seek(sentSize)) {
        qDebug() << "无法定位到文件位置" << sentSize << "错误：" << file.errorString();
        file.close();
        return;
    }
    // 读取分块数据
    const int bufferSize = 8192;
    int currentSize = std::min(bufferSize, (int)(fileSize - sentSize));
    qDebug()<<"currentSize:"<<currentSize<<" sentSize:"<<sentSize<<"Filesize:"<<fileSize;
    QByteArray buffer = file.read(currentSize);
    if (buffer.isEmpty()) {
        qDebug() << "文件读取失败：" << file.errorString();
        file.close();
        // return;
    }
    //qDebug()<<"currentSize:"<<currentSize;
    // 文件数据添加进数据包，回发结果客户端
    pack.addValue(MessagePackage::Key_SentSize, sentSize + currentSize);
    pack.addValue(MessagePackage::key_FileData, buffer);
    pack.setType(MessagePackage::Key_Type_FILEDATA);
    pack.sendMsg(socket);
    file.close(); // 关闭文件
}

void ClientSocketHandler::fileCancelUpload(MessagePackage &pack)
{
    // 找到文件存储路径
    QString sender = pack.getStringValue(MessagePackage::Key_Sender);
    QString receiver = pack.getStringValue(MessagePackage::Key_Receiver);
    QString fileName = pack.getStringValue(MessagePackage::key_FileName);
    qint64 fileSize = pack.getIntValue(MessagePackage::key_FileSize);
    int sentSize = pack.getIntValue(MessagePackage::Key_SentSize);
    QString path = "./fileData/";
    QString filePath;
    filePath = (pack.getIntValue(MessagePackage::Key_Result) == 1) ? "group" : "private";
    filePath += sender + receiver + fileName;
    path += filePath;
    qDebug() << "文件存储路径：" << path;

    // 打开文件
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件" << path << "错误：" << file.errorString();
        return;
    }
    if (!file.remove()) {
        qDebug() << "无法删除文件" << fileName<< "错误：" << file.errorString();
    }
}

void ClientSocketHandler::onDisConnected()
{
    emit clientOffline(this);
}

void ClientSocketHandler::handler(const MessagePackage &pack)
{
    MessagePackage tempPack = pack; // 创建一个非常量的副本
    qDebug()<<"handler going"<<tempPack.Type()<<" "<<socket->peerPort();
    auto msgHandler=getHandler(tempPack.Type());
    msgHandler(tempPack);
}

MsgHandler ClientSocketHandler::getHandler(QString msgType)
{
    auto it=msgHanderMap.find(msgType);
    if(it==msgHanderMap.end()){
        return [=](MessagePackage pack){
            qDebug()<<"not find handler: msgType";
        };
    }else{
        return msgHanderMap[msgType];
    }
}

void ClientSocketHandler::loginHandler(MessagePackage &pack)
{
    //获取账号和密码
    QString name=pack.getStringValue("name");
    QString pswd=pack.getStringValue("pswd");
    //数据库访问
    int ret=DBHelper::getInstance()->userLogin(name,pswd);
    pack.addValue("result",ret);
    //qDebug()<<"name:"<<name<<" pswd:"<<pswd<<"result:"<<pack.getIntValue("result");
    //发送给客户端
    pack.sendMsg(socket);
    if(pack.getIntValue("result")==1)
    {
        username=name;
        emit setClientName(this,name);//设置server管理客户端的map
        emit updateLists();//更新用户列表
    }

}

void ClientSocketHandler::logoutHandler(MessagePackage &pack)
{
    DBHelper::getInstance()->userOffline(username);
    emit updateLists();//更新用户列表
}

void ClientSocketHandler::registerHandler(MessagePackage &pack)
{
    QString name=pack.getStringValue(MessagePackage::Key_Name);
    QString pswd=pack.getStringValue(MessagePackage::Key_Pasd);
    int ret=DBHelper::getInstance()->userRegister(name,pswd);
    pack.addValue("result",ret);
    if(ret) emit updateLists();//更新用户列表
    pack.sendMsg(socket);
}

void ClientSocketHandler::updateUserListHandler(MessagePackage &pack)
{
    //qDebug()<<"name:"<<pack.getStringValue("name")<<" type:"<<pack.Type();
    QStringList list=DBHelper::getInstance()->getUsers();
    QStringList listg=DBHelper::getInstance()->getGroupNames(pack.getStringValue(MessagePackage::Key_Name));
    pack.addValue(MessagePackage::Key_UserList,list);
    pack.addValue(MessagePackage::Key_UserGroupList,listg);
    pack.sendMsg(socket);
}

void ClientSocketHandler::privateChatHandler(MessagePackage &pack)
{
    //qDebug()<<"send from"<<pack.getStringValue(MessagePackage::Key_Sender);
    emit privateMsg(pack);
}

void ClientSocketHandler::addGroupHandler(MessagePackage &pack)
{
    QString groupName=pack.getStringValue(MessagePackage::Key_GroupName);
    QString name=pack.getStringValue(MessagePackage::Key_Name);
    int ret=DBHelper::getInstance()->addGroup(groupName);
    DBHelper::getInstance()->addGroupMember(groupName,name);
    pack.addValue(MessagePackage::Key_Result,ret);
    pack.sendMsg(socket);
    emit updateLists();//更新用户列表
}

void ClientSocketHandler::inviteGroupHandler(MessagePackage &pack)
{
    QString groupName=pack.getStringValue(MessagePackage::Key_GroupName);
    QString name=pack.getStringValue(MessagePackage::Key_Name);
    DBHelper::getInstance()->addGroupMember(groupName,name);
    pack.sendMsg(socket);
    emit updateLists();//更新用户列表
    emit updateGroupMemberList(groupName);//更新用户群聊用户列表
}

void ClientSocketHandler::groupChatHandler(MessagePackage &Pack)
{
    emit groupMsg(Pack);
}

void ClientSocketHandler::getGroupMembersHandler(MessagePackage &pack)
{
    QStringList list=DBHelper::getInstance()->getGroupMembers(pack.getStringValue(MessagePackage::Key_GroupName));
    pack.addValue(MessagePackage::Key_UserList,list);
    pack.sendMsg(socket);
}

void ClientSocketHandler::privateFileHandler(MessagePackage &pack)
{
    //qDebug() << "get a file pack";
    //获取协议包信息
    QString sender=pack.getStringValue(MessagePackage::Key_Sender);//发送者
    QString receiver=pack.getStringValue(MessagePackage::Key_Receiver);//接受者
    QString filename=pack.getStringValue(MessagePackage::key_FileName);//文件名
    int filesize=pack.getIntValue(MessagePackage::key_FileSize);//文件大小
    int group=pack.getIntValue(MessagePackage::Key_Result);//是否是群文件
    //设置文件存储路径和添加数据库信息
    QString path="./fileData/";
    if(group==0){
        QString filePath="private"+sender+receiver+filename;
        path+=filePath;
        // DBHelper::getInstance()->addFile(path,filename,"private",sender,receiver,filesize);
    }
    else{
        QString filePath="group"+sender+receiver+filename;
        path+=filePath;
        // DBHelper::getInstance()->addFile(path,filename,"group",sender,receiver,filesize);
    }

    // 打开文件并预先分配磁盘空间
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法打开文件" << path << "错误：" << file.errorString();
        return;
    }

    // 预先分配文件大小
    if (!file.resize(filesize)) {
        qDebug() << "无法分配文件大小" << filesize << "字节，错误：" << file.errorString();
        file.close();
        return;
    }

    qDebug() << "文件" << path << "已成功分配" << filesize << "字节的磁盘空间";

    // 关闭文件
    file.close();
}

