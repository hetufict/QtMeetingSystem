#include <QMessageBox>
#include "chatinfo.h"
#include "ui_chatinfo.h"
#include <QFile>
#include <QFileInfo>
#include <QDialog>
#include <QInputDialog>
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
    updateUsrListRequest();
    connect(ui->list_UserList,&QListWidget::itemDoubleClicked,this,&ChatInfo::list_UserList_itemDoubleClicked);
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
    //QString reciver=pack.getStringValue(MessagePackage::Key_Receiver);//自己就是接收者
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
}

void ChatInfo::getPrivateMsg(const QString &objName, const QString &msg)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_PrivateChat);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_Receiver,objName);//发送对象
    pack.addValue(MessagePackage::Key_Message,msg);//内容
    //qDebug()<<"send to"<<objName;
    pack.sendMsg(socket);
}

void ChatInfo::onAddaddGroupRet(const MessagePackage &pack)
{
    int ret=pack.getIntValue(MessagePackage::Key_Result);
    QString groupName=pack.getStringValue(MessagePackage::Key_GroupName);
    if(ret)
    {
        QMessageBox::information(this,"新建群聊","新建群聊"+groupName+"成功");
    }
    else
    {
        QMessageBox::warning(this,"新建群聊","新建群聊"+groupName+"失败");
    }
}

void ChatInfo::onFlushGroupMembers(const QString &objname)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_GetGroupMembers);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_GroupName,objname);//发送对象
    pack.sendMsg(socket);
}

void ChatInfo::getGroupMsg(const QString &objName, const QString &msg)
{
    MessagePackage pack;
    QString objGroup = objName; // 假设 objName 已经正确传递
    pack.setType(MessagePackage::Key_Type_GroupChat); // 设置发送包类型
    pack.addValue(MessagePackage::Key_Sender, username); // 发送者
    pack.addValue(MessagePackage::Key_Receiver, objGroup); // 发送对象
    pack.addValue(MessagePackage::Key_Message, msg); // 内容

    qDebug() << "send to" << objGroup; // 调试输出，确认发送对象
    pack.sendMsg(socket); // 发送消息
}

void ChatInfo::onReceiveGroupChat(const MessagePackage &pack)
{
    QString sender=pack.getStringValue(MessagePackage::Key_Sender);//消息发送者
    QString receiver=pack.getStringValue(MessagePackage::Key_Receiver);//消息发送的群聊
    QString msg=pack.getStringValue(MessagePackage::Key_Message);
    qDebug()<<"group chat:"<<receiver;
    msg=sender+"says:"+msg;
    //QString objGroup="group"+receiver;
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
}

void ChatInfo::onReceiveGroupMembers(const MessagePackage &pack)
{
    QString receiver=pack.getStringValue(MessagePackage::Key_GroupName);//消息发送的群聊
    QStringList groupMembers=pack.getListValue(MessagePackage::Key_UserList);
    QString objGroup=receiver;
    qDebug()<<"get group members:"<<receiver;
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
}

void ChatInfo::onFileSendFinished() {
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
        return;
    }

    qint64 fileSize = file.size();
    QString fileName = QFileInfo(filePath).fileName();
    qDebug()<<"file:"<<fileName<<"size:"<<fileSize;
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
        filehelper=new FileHelper(true,group,filePath,fileName,username,objname);
        fileThrd=new QThread(this);

        // 连接信号和槽，确保文件发送任务在新线程中执行
        // 在 UserMenu 类中添加槽函数
        connect(this, &ChatInfo::startFileSendSignal, filehelper, &FileHelper::fileDeal);
        connect(filehelper, &FileHelper::fileSendFinished, this, &ChatInfo::onFileSendFinished);
        connect(fileThrd, &QThread::finished, filehelper, &QObject::deleteLater);
        connect(fileThrd, &QThread::finished, fileThrd, &QObject::deleteLater);

        filehelper->moveToThread(fileThrd);
        fileThrd->start();
        // 发送信号以启动文件发送任务
        emit startFileSendSignal();
    }else{
        QMessageBox::warning(this,"文件上传","请先完成当前文件上传或下载");
    }
}

void ChatInfo::onsendFileRespond(const MessagePackage &pack)
{
    QString filename=pack.getStringValue(MessagePackage::key_FileName);
    QMessageBox::information(this,"发送文件","发送文件"+filename+"成功");
}

void ChatInfo::onrecvPrivateFile(const MessagePackage &pack)
{
    QString sender=pack.getStringValue(MessagePackage::Key_Sender);//文件发送者
    QString filename=pack.getStringValue(MessagePackage::key_FileName);
    auto it = chatList.find(sender);
    if(it!=chatList.end())
    {
        ChatMenu* chat=it.value();
        chat->addFile(filename);
    }
    else
    {
        // 如果没有找到，创建一个新的 ChatMenu
        ChatMenu* chat = addNewChatMenu(false);
        chat->setObjName(sender); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(sender, chat);
        chat->addFile(filename);
    }
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
}

void ChatInfo::onReceiveFileList(const MessagePackage &pack)
{
    QString receiver=pack.getStringValue(MessagePackage::Key_Name);//消息发送的对象
    QStringList filelist=pack.getListValue(MessagePackage::key_FileList);
    QStringList senderlist=pack.getListValue(MessagePackage::key_SenderList);
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
        ChatMenu* chat = addNewChatMenu(true);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addFileList(filelist,senderlist);
    }
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
    //qDebug()<<group;
    pack.sendMsg(socket);
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
        qDebug() << "无法打开文件" << filePos << "错误：" << file.errorString();
        return;
    }

    // 预先分配文件大小
    if (!file.resize(filesize)) {
        qDebug() << "无法分配文件大小" << filesize << "字节，错误：" << file.errorString();
        file.close();
        return;
    }
    file.close();
    // 请求文件数据
    if(filehelper==nullptr&&fileThrd==nullptr){

        //下载，群文件，文件路径，文件名，发送者，接受者
        filehelper=new FileHelper(false,group,path,filename,fileSender,objname);
        fileThrd=new QThread(this);
        filehelper->setFileSize(filesize);
        // 连接信号和槽，确保文件发送任务在新线程中执行
        // 在 UserMenu 类中添加槽函数
        connect(this, &ChatInfo::startFileSendSignal, filehelper, &FileHelper::fileDeal);
        connect(filehelper, &FileHelper::fileSendFinished, this, &ChatInfo::onFileSendFinished);
        connect(fileThrd, &QThread::finished, filehelper, &QObject::deleteLater);
        connect(fileThrd, &QThread::finished, fileThrd, &QObject::deleteLater);
        filehelper->moveToThread(fileThrd);
        fileThrd->start();
        // 发送信号以启动文件发送任务
        emit startFileSendSignal();
    }else{
        QMessageBox::warning(this,"文件下载","请先完成当前文件上传或下载");
    }

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

void ChatInfo::on_pb_refllushList_clicked()
{
    updateUsrListRequest();
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
    return chat;
}
