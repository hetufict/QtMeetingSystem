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
    fileReader.reset();
    connect(socket,&QTcpSocket::readyRead,this,&ClientSocketHandler::onReadyRead);
    connect(socket,&QTcpSocket::disconnected,this,&ClientSocketHandler::onDisConnected);
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
void ClientSocketHandler::handleReceivedFile(const QByteArray &fileData, const QString &fileName,const QString &receiver) {
    //设置文件存储路径
    QString path="./fileData/";
    QString filePath;
    if(fileReader.group){
        filePath="group";
    }
    else{
        filePath="private";
    }
    filePath+=username+receiver+fileName;
    path+=filePath;
    qDebug() << "文件存储路径：" << path;
    //保存文件到服务器
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        // 使用 QFile 的 write 方法写入二进制数据
        file.write(fileData);
    } else {
        // 如果文件无法打开，显示错误消息
        qDebug() << "无法打开文件：" << path;
    }
    //关闭文件
    file.close();
}

void ClientSocketHandler::getFileListHandler(MessagePackage &pack)
{
    QString sender=pack.getStringValue(MessagePackage::Key_Sender);
    QString objname=pack.getStringValue(MessagePackage::Key_Name);
    qDebug()<<"??"<<objname;
    int group=pack.getIntValue(MessagePackage::Key_Result);
    QStringList filelist=DBHelper::getInstance()->getFileList(sender,objname,group);
    // for(auto file:filelist){
    //     qDebug()<<file<<"++++++++++++";
    // }
    QStringList senderList=DBHelper::getInstance()->getSenderList(filelist,sender,objname,group);
    // for(auto file:senderList){
    //     qDebug()<<"======="<<file<<"++++++++++++";
    // }
    pack.addValue(MessagePackage::key_FileList,filelist);
    pack.addValue(MessagePackage::key_SenderList,senderList);
    pack.sendMsg(socket);
}

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
    // 发送文件数据
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
    qDebug()<<"name"<<name<<"joined:"<<ret;
    if(ret){
        ret=DBHelper::getInstance()->joinMeeting(meetingID,meetingName,name);
        qDebug()<<"name"<<name<<"join:"<<ret;
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
    if(pack.getIntValue(MessagePackage::Key_Result)==1){
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
    filePath+=sender+receiver+fileName;
    path+=filePath;
    qDebug() << "文件存储路径：" << path;

    // 将接收到的数据添加到缓冲区
    fileBuffers[path].append(fileData);
    fileReadSize[path]+=fileData.size();

    // 如果缓冲区中的数据量达到阈值，写入文件
    if (fileBuffers[path].size() >= WRITE_THRESHOLD) {
        writeToFile(path, fileBuffers[path]);
        fileBuffers[path].clear();  // 清空缓冲区
    }
    if(fileReadSize[path]==filesize){
        fileTransferCompleted(path);
    }
}
void ClientSocketHandler::fileTransferCompleted(const QString &filePath)
{
    // 检查是否有剩余的缓冲区数据
    if (fileBuffers.contains(filePath) && !fileBuffers[filePath].isEmpty()) {
        writeToFile(filePath, fileBuffers[filePath]);
        fileBuffers.remove(filePath);  // 移除该文件的缓冲区
    }
    qDebug() << "文件传输完成，已保存所有数据到" << filePath;
}

void ClientSocketHandler::writeToFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    if (file.open(QIODevice::Append)) {
        // 使用 QFile 的 write 方法写入二进制数据
        file.write(data);
    } else {
        // 如果文件无法打开，显示错误消息
        qDebug() << "无法打开文件：" << path;
    }
    // 关闭文件
    file.close();
}

void ClientSocketHandler::onDisConnected()
{
    emit clientOffline(this);
}

void ClientSocketHandler::getFile(const MessagePackage &pack)
{
    // 获取文件大小和文件名
    QString filename=pack.getStringValue(MessagePackage::key_FileName);
    int filesize=pack.getIntValue(MessagePackage::key_FileSize);
    int group=pack.getIntValue(MessagePackage::Key_Result);
    QString receiver=pack.getStringValue(MessagePackage::Key_Receiver);
    qDebug()<<"server receiver file"<<filename<<"size"<<"=============="<<filesize;
    fileReader.fileName = filename;
    fileReader.size = filesize;
    fileReader.reciever=receiver;
    fileReader.readSoFar = 0;
    fileReader.fileData.clear();
    if(group)
    {
        fileReader.group=true;
    }
}

void ClientSocketHandler::acceptFile() {
    qDebug() << "file reading";
    // 确保剩余大小非负
    qint64 remainingSize = fileReader.size - fileReader.readSoFar;
    if (remainingSize < 0) {
        qDebug() << "错误：文件读取剩余大小为负数。";
        return; // 或者处理错误
    }
    qint64 bytesToRead = qMin(socket->bytesAvailable(), remainingSize);
    if (bytesToRead < 0) {
        qDebug() << "错误：读取的字节数为负数。";
        return; // 或者处理错误
    }
    QByteArray fileDataBlock = socket->read(bytesToRead);
    fileReader.fileData.append(fileDataBlock);
    fileReader.readSoFar += bytesToRead;
    if (fileReader.readSoFar == fileReader.size) {
        // 文件数据接收完成，处理文件
        handleReceivedFile(fileReader.fileData, fileReader.fileName, fileReader.reciever);
        fileReader.reset(); // 重置以准备接收下一个文件
    }
}

void ClientSocketHandler::handler(const MessagePackage &pack)
{
    MessagePackage tempPack = pack; // 创建一个非常量的副本
    qDebug()<<"handler going"<<pack.Type()<<" "<<socket->peerPort();
    if(pack.Type()==MessagePackage::Key_Type_Login)//登陆处理
    {
        loginHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_Logout)//登出处理
    {
        logoutHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_Register)//注册处理
    {
        registerHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_UpdateList)//获取用户列表处理
    {
        updateUserListHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_PrivateChat)//私聊请求处理
    {
        privateChatHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_AddGroup){//新建群聊请求处理
        addGroupHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_InviteGroupMember)//邀请群员请求
    {
        inviteGroupHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_GroupChat)//群聊消息处理
    {
        groupChatHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_GetGroupMembers)//群聊消息处理
    {
        getGroupMembersHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_PrivateFile)//私聊文件处理
    {
        privateFileHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_GetFileList)//私聊文件处理
    {
        getFileListHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_GetFile)//文件下载处理
    {
        getFileHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_CreateMeeting)//创建会议
    {
        createMeetingHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_InviteMeeting)//创建会议
    {
        inviteMeetingHandler(tempPack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_JoinMeeting)//创建会议
    {
        joinMeetingHandler(tempPack);
    }else if(pack.Type()==MessagePackage::Key_Type_CloseMeeting){
        closeMeetingHandler(tempPack);
    }else if(pack.Type()==MessagePackage::Key_Type_MembersList){
        membersListHandler(tempPack);
    }else if(pack.Type()==MessagePackage::Key_Type_FILEDATA){
        fileDataHandler(tempPack);
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
        emit setClientName(this,name);
    }

}

void ClientSocketHandler::logoutHandler(MessagePackage &pack)
{
    DBHelper::getInstance()->userOffline(username);
}

void ClientSocketHandler::registerHandler(MessagePackage &pack)
{
    QString name=pack.getStringValue("name");
    QString pswd=pack.getStringValue("pswd");
    int ret=DBHelper::getInstance()->userRegister(name,pswd);
    pack.addValue("result",ret);
    //qDebug()<<"name:"<<name<<" pswd:"<<pswd<<"result:"<<pack.getIntValue("result");
    pack.sendMsg(socket);
}

void ClientSocketHandler::updateUserListHandler(MessagePackage &pack)
{
    //qDebug()<<"name:"<<pack.getStringValue("name")<<" type:"<<pack.Type();
    QStringList list=DBHelper::getInstance()->getUsers();
    QStringList listg=DBHelper::getInstance()->getGroupNames(pack.getStringValue("name"));
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
}

void ClientSocketHandler::inviteGroupHandler(MessagePackage &pack)
{
    QString groupName=pack.getStringValue(MessagePackage::Key_GroupName);
    QString name=pack.getStringValue(MessagePackage::Key_Name);
    DBHelper::getInstance()->addGroupMember(groupName,name);
    pack.sendMsg(socket);
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
    //下载文件处理(解析协议包开始接受文件)
    getFile(pack);
    //设置文件存储路径和添加数据库信息
    QString path="./fileData/";
    if(group==0){
        QString filePath="private"+sender+receiver+filename;
        path+=filePath;
        DBHelper::getInstance()->addFile(path,filename,"private",sender,receiver,filesize);
    }
    else{
        QString filePath="group"+sender+receiver+filename;
        path+=filePath;
        DBHelper::getInstance()->addFile(path,filename,"group",sender,receiver,filesize);
    }
    //设置回发数据包，(发送者上传成功应答，接受者获得文件信息)
    MessagePackage respack;
    respack.setType(MessagePackage::Key_Type_PrivateFile); // 设置发送包类型
    respack.addValue(MessagePackage::Key_Sender, sender); // 发送者
    respack.addValue(MessagePackage::Key_Receiver, receiver); // 发送对象
    respack.addValue(MessagePackage::key_FileName, filename); // 发送的文件名
    respack.sendMsg(socket); // 发送消息
    respack.addValue(MessagePackage::Key_Result, 0); // 区分发送者和接受者
    qDebug() << "send file result to" << sender;
    emit privateFile(respack);
}

