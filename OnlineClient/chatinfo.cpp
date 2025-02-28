#include <QFile>
#include <QFileInfo>
#include <QDialog>
#include <QInputDialog>
#include <QMessageBox>
#include "chatinfo.h"
#include "ui_chatinfo.h"
#include "logger.h"
ChatInfo::ChatInfo(QWidget *parent,QTcpSocket* socket,QString username)
    : QWidget(parent)
    , ui(new Ui::ChatInfo)
    ,socket(socket)
    ,username(username)
    ,curMenu(nullptr)
    ,timer(nullptr)
{
    ui->setupUi(this);
    frameLayout=new QVBoxLayout;
    ui->frame->setLayout(frameLayout);
    //updateUsrListRequest();
    connect(ui->list_UserList,&QListWidget::itemDoubleClicked,this,&ChatInfo::list_UserList_itemDoubleClicked);
    //恢复文件传输线程
    recoverFileProcess();
}

ChatInfo::~ChatInfo()
{
    delete ui;
}

void ChatInfo::updateUsrListRequest()
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_UpdateList);
    pack.addValue(MessagePackage::Key_Name,username);
    pack.sendMsg(socket);
}

QString ChatInfo::getUsername()
{
    return username;
}

void ChatInfo::updateUserList(const MessagePackage &pack)
{
    // 清空当前列表
    ui->list_UserList->clear();
    ui->list_groupList->clear();
    QStringList list=pack.getListValue(MessagePackage::Key_UserList);
    QStringList listgroup=pack.getListValue(MessagePackage::Key_UserGroupList);
    for(const auto& it : list)
    {
        ui->list_UserList->addItem(it);
    }
    for(const auto& it : listgroup)
    {
        ui->list_groupList->addItem(it);
    }
}

void ChatInfo::onReceivePrivateChat(const MessagePackage &pack)
{
    QString sender=pack.getStringValue(MessagePackage::Key_Sender);
    QString msg=pack.getStringValue(MessagePackage::Key_Message);
    auto it = chatList.find(sender);
    if(it!=chatList.end())
    {
        ChatMenu* chat=it.value();
        chat->addMsgText(msg);
    }
    else
    {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat = addNewChatMenu(false);
        chat->setObjName(sender); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(sender, chat);
        chat->addMsgText(msg);
    }
    LOG(Logger::Info,"receive parivate message");
}

void ChatInfo::getPrivateMsg(const QString &objName, const QString &msg)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_PrivateChat);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_Receiver,objName);//发送对象
    pack.addValue(MessagePackage::Key_Message,msg);//内容
    pack.sendMsg(socket);
    LOG(Logger::Info,"send parivate message to "+objName);
}

void ChatInfo::onAddaddGroupRet(const MessagePackage &pack)
{
    int ret=pack.getIntValue(MessagePackage::Key_Result);
    QString groupName=pack.getStringValue(MessagePackage::Key_GroupName);
    if(ret)
    {
        QMessageBox::information(this,"新建群聊","新建群聊"+groupName+"成功");
            LOG(Logger::Info,"created a group "+groupName);
    }
    else
    {
        QMessageBox::warning(this,"新建群聊","新建群聊"+groupName+"失败");
            LOG(Logger::Info,"failed to create a group "+groupName);
    }
}

void ChatInfo::onFlushGroupMembers(const QString &objname)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_GetGroupMembers);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_GroupName,objname);//发送对象
    pack.sendMsg(socket);
    LOG(Logger::Info,"request group memberlist");
}

void ChatInfo::getGroupMsg(const QString &objName, const QString &msg)
{
    MessagePackage pack;
    QString objGroup = objName; // 假设 objName 已经正确传递
    pack.setType(MessagePackage::Key_Type_GroupChat); // 设置发送包类型
    pack.addValue(MessagePackage::Key_Sender, username); // 发送者
    pack.addValue(MessagePackage::Key_Receiver, objGroup); // 发送对象
    pack.addValue(MessagePackage::Key_Message, msg); // 内容
    pack.sendMsg(socket); // 发送消息
    LOG(Logger::Info, QString("send a group message group: %1").arg(objGroup));
}

void ChatInfo::onReceiveGroupChat(const MessagePackage &pack)
{
    QString sender=pack.getStringValue(MessagePackage::Key_Sender);//消息发送者
    QString receiver=pack.getStringValue(MessagePackage::Key_Receiver);//消息发送的群聊
    QString msg=pack.getStringValue(MessagePackage::Key_Message);
    msg=sender+"says:"+msg;
    QString objGroup=receiver;
    auto it = chatList.find(objGroup);
    if(it!=chatList.end())
    {
        ChatMenu* chat=it.value();
        chat->addMsgText(msg);
    }
    else
    {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat = addNewChatMenu(true);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addMsgText(msg);
    }
    LOG(Logger::Info, QString("receive a group message group: %1").arg(objGroup));
}

void ChatInfo::onReceiveGroupMembers(const MessagePackage &pack)
{
    QString receiver=pack.getStringValue(MessagePackage::Key_GroupName);//消息发送的群聊
    QStringList groupMembers=pack.getListValue(MessagePackage::Key_UserList);
    QString objGroup=receiver;
    auto it = chatList.find(objGroup);
    if(it!=chatList.end())
    {
        ChatMenu* chat=it.value();
        chat->addGroupMembers(groupMembers);
    }
    else
    {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat = addNewChatMenu(true);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addGroupMembers(groupMembers);
    }
    LOG(Logger::Info, QString("receive a group member list group: %1").arg(objGroup));
}
//文件线程结束
void ChatInfo::onFileSendFinished(MessagePackage& pack) {
    if(filehelper->getSend()&&!filehelper->getCancel()){//文件上传成功
        QString filename=pack.getStringValue(MessagePackage::key_FileName);//文件名
        QString receiver=pack.getStringValue(MessagePackage::Key_Receiver);//消息发送的对象
        QStringList filelist=pack.getListValue(MessagePackage::key_FileList);//更新文件列表
        QStringList senderlist=pack.getListValue(MessagePackage::key_SenderList);//文件发送者列表
        QString objGroup=receiver;
        int group=pack.getIntValue(MessagePackage::Key_Result);
        auto it = chatList.find(receiver);
        if(it!=chatList.end())
        {
            ChatMenu* chat=it.value();
            chat->addFileList(filelist,senderlist);
        }
        else
        {
            // 如果没有找到，创建一个新的 ChatMenu
            ChatMenu* chat = addNewChatMenu(group);
            chat->setObjName(objGroup); // 设置聊天对象名称
            // 将新的 ChatMenu 添加到 chatList 中
            chatList.insert(objGroup, chat);
            chat->addFileList(filelist,senderlist);
        }
        QMessageBox::information(this,"发送文件","发送文件"+filename+"成功");
        LOG(Logger::Info,"send file:"+filename+" succeed");
    }
    if(!filehelper->getCancel()){
        emit finishedFile(filehelper->getFileName(),filehelper->getSend());

    }else{//取消当前文件操作
        emit setCancelFile(filehelper->getFileName(),filehelper->getSend());
        LOG(Logger::Info,"file:"+filehelper->getFileName()+" cancel");
    }
    if (fileThrd) {
        fileThrd->quit(); // 结束线程事件循环
        fileThrd->wait(); // 等待线程结束
        fileThrd = nullptr;
        filehelper = nullptr;
    }
}

void ChatInfo::onFileSent(const QString &objname, const QString &filePath,bool group) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << filePath;
        QMessageBox::warning(this,"打开文件","打开文件失败");
        LOG(Logger::Error,"open file errror: "+filePath);
        return;
    }

    qint64 fileSize = file.size();
    if(fileSize==0){
        QMessageBox::warning(this,"打开文件","打开文件的大小为0");
        LOG(Logger::Warning,"open file error:size = 0"+filePath);
        return;
    }
    QString fileName = QFileInfo(filePath).fileName();
    // 准备协议包
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_PrivateFile); // 设置发送包类型
    pack.addValue(MessagePackage::Key_Sender, username); // 发送者
    pack.addValue(MessagePackage::Key_Receiver, objname); // 发送对象
    pack.addValue(MessagePackage::key_FileName, fileName); // 发送的文件名
    pack.addValue(MessagePackage::key_FileSize, fileSize); // 发送的文件大小
    if(group)
    {
        pack.addValue(MessagePackage::Key_Result, 1);
    }else{
        pack.addValue(MessagePackage::Key_Result, 0);
    }
    // 发送协议包
    pack.sendMsg(socket);

    file.close();
    // 发送文件数据
    if(filehelper==nullptr&&fileThrd==nullptr){

        createFilehelper(true,group,filePath,fileName,username,objname,fileSize);
        filehelper->moveToThread(fileThrd);
        fileThrd->start();
        // 发送信号以启动文件发送任务
        emit startFileSendSignal();
    }else{
        QMessageBox::warning(this,"文件上传","请先完成当前文件上传或下载");
        LOG(Logger::Warning,"send file :file thread is alive");
    }
}

void ChatInfo::onUpdateLists(const MessagePackage &pack) {
    // 清空当前列表
    ui->list_UserList->clear();
    ui->list_groupList->clear();

    // 获取在线用户和离线用户列表
    QStringList onlineUsers = pack.getListValue(MessagePackage::Key_OnlineUsers);
    QStringList offlineUsers = pack.getListValue(MessagePackage::Key_OfflineUsers);
    QStringList listgroup = pack.getListValue(MessagePackage::Key_UserGroupList);
    // 设置成员在会议中的背景色
    QColor colorOnline = QColor(0, 255, 0); // 绿色
    // 设置成员离开会议的背景色
    QColor colorOffline = QColor(255, 165, 0); // 橙色

    // 添加在线用户到用户列表（绿色）
    for (const auto& user : onlineUsers) {
        QListWidgetItem* item = new QListWidgetItem(user);
        item->setBackground(QBrush(colorOnline ));
        item->setTextAlignment(Qt::AlignCenter); // 设置文本居中
        ui->list_UserList->addItem(item);
    }
    // 添加离线用户到用户列表（黄色）
    for (const auto& user : offlineUsers) {
        QListWidgetItem* item = new QListWidgetItem(user);
        item->setBackground(QBrush(colorOffline));
        item->setTextAlignment(Qt::AlignCenter); // 设置文本居中
        ui->list_UserList->addItem(item);
    }
    // 添加群组到群组列表
    for (const auto& group : listgroup) {
        QListWidgetItem* item = new QListWidgetItem(group);
        item->setTextAlignment(Qt::AlignCenter); // 设置文本居中
        ui->list_groupList->addItem(item);
    }
    LOG(Logger::Info,"flush userlist and grouplist");
}

void ChatInfo::onsendFileRespond(const MessagePackage &pack)
{
    QString filename=pack.getStringValue(MessagePackage::key_FileName);
    QString receiver=pack.getStringValue(MessagePackage::Key_Receiver);//消息发送的对象
    QStringList filelist=pack.getListValue(MessagePackage::key_FileList);
    QStringList senderlist=pack.getListValue(MessagePackage::key_SenderList);
    QString objGroup=receiver;
    int group=pack.getIntValue(MessagePackage::Key_Result);
    auto it = chatList.find(receiver);
    if(it!=chatList.end())
    {
        ChatMenu* chat=it.value();
        chat->addFileList(filelist,senderlist);
    }
    else
    {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat = addNewChatMenu(group);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addFileList(filelist,senderlist);
    }
    // QMessageBox::information(this,"发送文件","发送文件"+filename+"成功");
}

void ChatInfo::onrecvPrivateFile(const MessagePackage &pack)
{
    //QString filename=pack.getStringValue(MessagePackage::key_FileName);
    QString receiver=pack.getStringValue(MessagePackage::Key_Sender);//消息发送的对象
    QStringList filelist=pack.getListValue(MessagePackage::key_FileList);
    QStringList senderlist=pack.getListValue(MessagePackage::key_SenderList);
    int group=pack.getIntValue(MessagePackage::Key_Result);
    QString objGroup=receiver;
    auto it = chatList.find(receiver);
    if(it!=chatList.end())
    {
        ChatMenu* chat=it.value();
        chat->addFileList(filelist,senderlist);
    }
    else
    {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat = addNewChatMenu(group);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addFileList(filelist,senderlist);
    }
    //QMessageBox::information(this,"发送文件","发送文件"+filename+"成功");
}

void ChatInfo::onFlushFileList(const QString &objname,bool group)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_GetFileList);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_Name,objname);//发送对象
    if(group)
    {
        pack.addValue(MessagePackage::Key_Result, 1);
    }else{
        pack.addValue(MessagePackage::Key_Result, 0);
    }
    pack.sendMsg(socket);
    LOG(Logger::Info,"request flush filelist");
}

void ChatInfo::onReceiveFileList(const MessagePackage &pack)
{
    QString receiver=pack.getStringValue(MessagePackage::Key_Name);//消息发送的对象
    QStringList filelist=pack.getListValue(MessagePackage::key_FileList);
    QStringList senderlist=pack.getListValue(MessagePackage::key_SenderList);
    int group=pack.getIntValue(MessagePackage::Key_Result);
    QString objGroup=receiver;
    auto it = chatList.find(receiver);
    if(it!=chatList.end())
    {
        ChatMenu* chat=it.value();
        chat->addFileList(filelist,senderlist);
    }
    else
    {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat = addNewChatMenu(group);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addFileList(filelist,senderlist);
    }
    LOG(Logger::Info,"receive a file list");
}

void ChatInfo::onRequestFile(const QString &filename, const QString &senderName, const QString &objName,bool group)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_GetFile);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_Receiver,objName);//聊天的对象
    pack.addValue(MessagePackage::key_FileName,filename);//文件
    pack.addValue(MessagePackage::Key_Name,senderName);//文件的发送者
    if(group)
    {
        pack.addValue(MessagePackage::Key_Result, 1);
    }else{
        pack.addValue(MessagePackage::Key_Result, 0);
    }
    pack.sendMsg(socket);
    LOG(Logger::Info,"request a file");
}

//提示文件是否可以下载，预开辟文件空间，开辟线程，请求文件数据
void ChatInfo::onReceiveFile(const MessagePackage &pack)
{
    //QString packSender=pack.getStringValue(MessagePackage::Key_Sender);
    QString objname=pack.getStringValue(MessagePackage::Key_Receiver);
    QString fileSender=pack.getStringValue(MessagePackage::Key_Name);
    QString filename=pack.getStringValue(MessagePackage::key_FileName);
    int group=pack.getIntValue(MessagePackage::Key_Result);
    int filesize=pack.getIntValue(MessagePackage::key_FileSize);
    QString path="./fileData/";
    QString filePos=path+'/'+filename;
    // 打开文件并预先分配磁盘空间
    QFile file(filePos);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG(Logger::Error,"open file failed "+filePos+file.errorString());
        return;
    }

    // 预先分配文件大小
    if (!file.resize(filesize)) {
        LOG(Logger::Error,"resize file failed "+filePos+file.errorString());
        file.close();
        return;
    }
    file.close();
    // 请求文件数据
    if(filehelper==nullptr&&fileThrd==nullptr){

        createFilehelper(false,group,path,filename,fileSender,objname,filesize);
        filehelper->moveToThread(fileThrd);
        fileThrd->start();
        // 发送信号以启动文件发送任务
        emit startFileSendSignal();
    }else{
        QMessageBox::warning(this,"文件下载","请先完成当前文件上传或下载");
        LOG(Logger::Warning,"requst a file :file thread is alive");
    }

}

void ChatInfo::onCancelFile()
{
    emit cancelFile();
}

void ChatInfo::onPauseFile()
{
    emit pauseFile();
}

void ChatInfo::onFileSpeed(double progressPercentage,double speed, QString fileName, bool send)
{
    emit updateFileSpeed(progressPercentage,speed,fileName,send);
}


void ChatInfo::list_UserList_itemDoubleClicked(QListWidgetItem *item)
{
    QString objName = item->text();
    auto it = chatList.find(objName);

    // 隐藏当前显示的 ChatMenu
    if (curMenu && curMenu->isVisible()) {
        curMenu->hide();
    }

    if (it != chatList.end()) {
        // 如果找到了对应的 ChatMenu
        ChatMenu* chat = it.value();
        // 显示被双击的 ChatMenu
        frameLayout->removeWidget(curMenu); // 移除当前 ChatMenu
        frameLayout->addWidget(chat); // 添加被双击的 ChatMenu
        chat->show();
        // 更新当前活动的 ChatMenu
        curMenu = chat;
    } else {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat = addNewChatMenu(false);
        chat->setObjName(objName); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objName, chat);
        // 显示新的 ChatMenu
        frameLayout->addWidget(chat); // 添加新的 ChatMenu
        chat->show();
        // 更新当前活动的 ChatMenu
        curMenu = chat;
    }
}

void ChatInfo::on_pb_newgroup_clicked()
{
    // 显示对话框获取群名
    bool ok;
    QString groupName = QInputDialog::getText(this, tr("新建群组"), tr("请输入群组名称:"), QLineEdit::Normal, "", &ok);

    // 检查用户是否点击了 OK 按钮并且输入了群名
    if (ok && !groupName.isEmpty()){
        // 打包群名发送到服务器
        MessagePackage pack;
        pack.setType(MessagePackage::Key_Type_AddGroup);
        pack.addValue(MessagePackage::Key_Name,username);
        pack.addValue(MessagePackage::Key_GroupName,groupName);
        pack.sendMsg(socket);
        LOG(Logger::Info,"add new group requst group: "+groupName);
    }
    else
    {
        QMessageBox::warning(this,"设置群名","群组名字不能为空");
    }
}


void ChatInfo::on_list_groupList_itemDoubleClicked(QListWidgetItem *item)
{
    //添加群组前缀区分群名和用户名
    //QString objName ="group"+item->text();
    QString objName =item->text();
    auto it = chatList.find(objName);

    // 隐藏当前显示的 ChatMenu
    if (curMenu && curMenu->isVisible()) {
        curMenu->hide();
    }

    if (it != chatList.end()) {
        // 如果找到了对应的 ChatMenu
        ChatMenu* chat = it.value();
        // 显示被双击的 ChatMenu
        frameLayout->removeWidget(curMenu); // 移除当前 ChatMenu
        frameLayout->addWidget(chat); // 添加被双击的 ChatMenu
        chat->show();
        // 更新当前活动的 ChatMenu
        curMenu = chat;
    } else {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat =addNewChatMenu(true); // 设置 ui->frame 作为父对象
        chat->setObjName(objName); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objName, chat);
        // 显示新的 ChatMenu
        frameLayout->addWidget(chat); // 添加新的 ChatMenu
        chat->show();
        // 更新当前活动的 ChatMenu
        curMenu = chat;
    }
}

ChatMenu *ChatInfo::addNewChatMenu(bool group)
{
    ChatMenu* chat = new ChatMenu(ui->frame,group); // 设置 ui->frame 作为父对象
    connect(chat,&ChatMenu::sendPrivateChat,this,&ChatInfo::getPrivateMsg);
    connect(chat,&ChatMenu::sendGroupChat,this,&ChatInfo::getGroupMsg);
    connect(chat,&ChatMenu::fileSent,this,&ChatInfo::onFileSent);
    connect(chat,&ChatMenu::flushGrroupMembers,this,&ChatInfo::onFlushGroupMembers);
    connect(chat,&ChatMenu::flushFileList,this,&ChatInfo::onFlushFileList);
    connect(chat,&ChatMenu::requestFile,this,&ChatInfo::onRequestFile);
    connect(chat,&ChatMenu::cancelFile,this,&ChatInfo::onCancelFile);
    connect(chat,&ChatMenu::pauseFile,this,&ChatInfo::onPauseFile);
    connect(this,&ChatInfo::updateFileSpeed,chat,&ChatMenu::setFileSpeed);
    connect(this,&ChatInfo::finishedFile,chat,&ChatMenu::setFileFinished);
    connect(this,&ChatInfo::setCancelFile,chat,&ChatMenu::setCancelfile);
    return chat;
}

void ChatInfo::createFilehelper(const bool &send, const bool &group, const QString &filePath,
                                const QString &filename, const QString &sender, const QString &recer, const qint64 &fileSize)
{
    //下载，群文件，文件路径，文件名，发送者，接受者
    filehelper=new FileHelper(send,group,filePath,filename,sender,recer);
    fileThrd=new QThread(this);
    filehelper->setFileSize(fileSize);
    // 连接信号和槽，确保文件发送任务在新线程中执行
    // 在 UserMenu 类中添加槽函数
    connect(this, &ChatInfo::startFileSendSignal, filehelper, &FileHelper::fileDeal);
    connect(filehelper, &FileHelper::fileSendFinished, this, &ChatInfo::onFileSendFinished);
    connect(fileThrd, &QThread::finished, filehelper, &QObject::deleteLater);
    connect(fileThrd, &QThread::finished, fileThrd, &QObject::deleteLater);
    //文件暂停 取消 进度
    connect(this,&ChatInfo::cancelFile,filehelper,&FileHelper::onCancelFile);
    connect(this,&ChatInfo::pauseFile,filehelper,&FileHelper::onPauseFile);
    connect(filehelper,&FileHelper::transferSpeedUpdated,this,&ChatInfo::onFileSpeed);
}

void ChatInfo::recoverFileProcess() {
    QString logFileName = "file_process.txt";
    QFile log(logFileName);

    // 检查日志文件是否存在
    if (!log.exists()) {
        LOG(Logger::Error,"file_prpcess.txt open failed: not exist");
        //qDebug() << "日志文件不存在，可能是传输已完成或从未开始";
        return;
    }

    // 打开日志文件
    if (!log.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG(Logger::Error,"file_prpcess.txt open failed:"+log.errorString());
        //qDebug() << "无法打开日志文件：" << log.errorString();
        return;
    }

    // 读取日志文件内容
    QTextStream in(&log);
    QString logText = in.readLine(); // 假设日志文件只有一行

    // 解析日志文件内容
    QStringList logItems = logText.split(';');
    QMap<QString, QString> logData;
    for (const QString &item : logItems) {
        QStringList keyValue = item.split('=');
        if (keyValue.size() == 2) {
            logData[keyValue[0]] = keyValue[1];
        }
    }
    // 检索日志数据
    bool send = logData.value("send").toInt();
    bool group = logData.value("group").toInt();
    QString filePath = logData.value("filePath");
    QString fileName = logData.value("fileName");
    QString sender = logData.value("sender");
    QString receiver = logData.value("receiver");
    qint64 sentSize = logData.value("sentSize").toLongLong();
    qint64 fileSize = logData.value("fileSize").toLongLong();
    // double progress = logData.value("progress").toDouble();
    if(filehelper==nullptr&&fileThrd==nullptr){
        createFilehelper(send,group,filePath,fileName,sender,receiver,fileSize);
        filehelper->setSentSize(sentSize);
        filehelper->setPause(true);

        filehelper->moveToThread(fileThrd);
        fileThrd->start();
        // 发送信号以启动文件发送任务
        emit startFileSendSignal();
    }else{
        return ;
    }
}





