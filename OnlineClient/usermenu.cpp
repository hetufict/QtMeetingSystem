#include "usermenu.h"
#include "ui_usermenu.h"
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QTableWidget>
UserMenu::UserMenu(QWidget *parent,QTcpSocket* socket,QString username)
    : QMainWindow(parent)
    , ui(new Ui::UserMenu)
    ,socket(socket)
    ,username(username)
    ,curMenu(nullptr)
    ,mr_w(nullptr)
    ,timer(nullptr)
{
    ui->setupUi(this);
    frameLayout=new QVBoxLayout;
    ui->frame->setLayout(frameLayout);
    updateUsrListRequest();
    connect(ui->list_UserList,&QListWidget::itemDoubleClicked,this,&UserMenu::list_UserList_itemDoubleClicked);
}

UserMenu::~UserMenu()
{
    delete ui;
}

//发送获取用户列表请求
void UserMenu::updateUsrListRequest()
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_UpdateList);
    pack.addValue(MessagePackage::Key_Name,username);
    pack.sendMsg(socket);
}

QString UserMenu::getUsername()
{
    return username;
}

void UserMenu::requestMeetingMembers()
{
    if(mr_w){//如果会议开启则发送请求
        MessagePackage pack;
        pack.setType(MessagePackage::Key_Type_MembersList);//设置请求类型
        pack.addValue(MessagePackage::Key_Name,username);
        pack.addValue(MessagePackage::key_MeetingID,mr_w->getMeetingID());//会议ID
        pack.addValue(MessagePackage::key_MeetingName,mr_w->getMeetingName());//会议名
        pack.sendMsg(socket);
    }
}

//更新用户列表
void UserMenu::updateUserList(const MessagePackage &pack)
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
//更新私聊信息
void UserMenu::onReceivePrivateChat(const MessagePackage &pack)
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
        ChatMenu* chat = addNewCharMenu(false);
        chat->setObjName(sender); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(sender, chat);
        chat->addMsgText(msg);
    }
}
//获取对话框私聊信息并发送
void UserMenu::getPrivateMsg(const QString &objName, const QString &msg)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_PrivateChat);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_Receiver,objName);//发送对象
    pack.addValue(MessagePackage::Key_Message,msg);//内容
    //qDebug()<<"send to"<<objName;
    pack.sendMsg(socket);
}

void UserMenu::onAddaddGroupRet(const MessagePackage &pack)
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

void UserMenu::onFlushGroupMembers(const QString &objname)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_GetGroupMembers);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_GroupName,objname);//发送对象
    pack.sendMsg(socket);
}
//获取群聊窗口的消息，发送给服务器
void UserMenu::getGroupMsg(const QString &objName, const QString &msg) {
    MessagePackage pack;
    QString objGroup = objName; // 假设 objName 已经正确传递
    pack.setType(MessagePackage::Key_Type_GroupChat); // 设置发送包类型
    pack.addValue(MessagePackage::Key_Sender, username); // 发送者
    pack.addValue(MessagePackage::Key_Receiver, objGroup); // 发送对象
    pack.addValue(MessagePackage::Key_Message, msg); // 内容

    qDebug() << "send to" << objGroup; // 调试输出，确认发送对象
    pack.sendMsg(socket); // 发送消息
}
//解析接受的群聊消息包
void UserMenu::onReceiveGroupChat(const MessagePackage &pack)
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
        ChatMenu* chat = addNewCharMenu(true);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addMsgText(msg);
    }
}

void UserMenu::onReceiveGroupMembers(const MessagePackage &pack)
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
        ChatMenu* chat = addNewCharMenu(true);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addGroupMembers(groupMembers);
    }
}
// 在 UserMenu 类中添加槽函数
void UserMenu::onFileSendFinished() {
    if (fileThrd) {
        fileThrd->quit(); // 结束线程事件循环
        fileThrd->wait(); // 等待线程结束
        fileThrd = nullptr;
        filehelper = nullptr;
    }
}

void UserMenu::onFileSent(const QString &objname, const QString &filePath,bool group) {
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
        connect(this, &UserMenu::startFileSendSignal, filehelper, &FileHelper::fileDeal);
        connect(filehelper, &FileHelper::fileSendFinished, this, &UserMenu::onFileSendFinished);
        connect(fileThrd, &QThread::finished, filehelper, &QObject::deleteLater);
        connect(fileThrd, &QThread::finished, fileThrd, &QObject::deleteLater);

        filehelper->moveToThread(fileThrd);
        fileThrd->start();
        // 发送信号以启动文件发送任务
        emit startFileSendSignal();
    }else{
        QMessageBox::warning(mr_w,"文件上传","请先完成当前文件上传或下载");
    }
}

void UserMenu::onsendFileRespond(const MessagePackage &pack)
{
    QString filename=pack.getStringValue(MessagePackage::key_FileName);
    QMessageBox::information(this,"发送文件","发送文件"+filename+"成功");
}

void UserMenu::onrecvPrivateFile(const MessagePackage &pack)
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
        ChatMenu* chat = addNewCharMenu(false);
        chat->setObjName(sender); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(sender, chat);
        chat->addFile(filename);
    }
}

void UserMenu::onFlushFileList(const QString &objname,bool group)
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

void UserMenu::onReceiveFileList(const MessagePackage &pack)
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
        ChatMenu* chat = addNewCharMenu(true);
        chat->setObjName(objGroup); // 设置聊天对象名称
        // 将新的 ChatMenu 添加到 chatList 中
        chatList.insert(objGroup, chat);
        chat->addFileList(filelist,senderlist);
    }
}

void UserMenu::onRequestFile(const QString &filename, const QString &senderName, const QString &objName,bool group)
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
void UserMenu::onReceiveFile(const MessagePackage &pack)
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
        connect(this, &UserMenu::startFileSendSignal, filehelper, &FileHelper::fileDeal);
        connect(filehelper, &FileHelper::fileSendFinished, this, &UserMenu::onFileSendFinished);
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

void UserMenu::onCreateMeeting(const MessagePackage &pack)
{
    int ret=pack.getIntValue(MessagePackage::key_MeetingID);
    if(ret!=-1)
    {
        QString meetname=pack.getStringValue(MessagePackage::key_MeetingName);
        QString hostname=pack.getStringValue(MessagePackage::Key_Name);
        int messageport=pack.getIntValue(MessagePackage::Key_MessagePort);
        int videoport=pack.getIntValue(MessagePackage::Key_VideoPort);
        int mediaport=pack.getIntValue(MessagePackage::Key_MediaPort);
        addMeetingList(meetname,ret);
        qDebug()<<ret<<"===="<<messageport<<"=="<<videoport<<"=="<<mediaport;
        if(!mr_w){
            timer=new QTimer(this);
            connect(timer,&QTimer::timeout,this,&UserMenu::requestMeetingMembers);
            timer->start(3000);//定时器，定时刷新会议成员列表
            //初始化会议菜单
            mr_w=new MeetingRoom(meetname,username,hostname,ret,messageport,videoport,mediaport);
            connect(mr_w,&MeetingRoom::invited,this,&UserMenu::oninvited);
            connect(mr_w,&MeetingRoom::meetingRoomClose,this,&UserMenu::onMeetingRoomClose);
            mr_w->setWindowTitle(meetname);
            mr_w->show();
        }
    }
    else{

    }
}

void UserMenu::oninvited(const QString &name, const QString &meetName,int& meetingID)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_InviteMeeting);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_Receiver,name);//邀请的对象
    pack.addValue(MessagePackage::key_MeetingName,meetName);//会议名
    pack.addValue(MessagePackage::key_MeetingID,meetingID);//会议id
    pack.sendMsg(socket);
}

void UserMenu::onInviteMeetingRespond(const MessagePackage &pack)
{
    int ret=pack.getIntValue(MessagePackage::Key_Result);
    if(ret){
        QMessageBox::information(mr_w,"邀请会议成员","邀请成功");
    }else{
        QMessageBox::warning(mr_w,"邀请会议成员","未找到该成员");
    }
}

void UserMenu::onGetMeetingInvitation(const MessagePackage &pack)
{
    //qDebug()<<"Get A Meeting Invitation";
    QString meetingName = pack.getStringValue(MessagePackage::key_MeetingName);
    int meetingId = pack.getIntValue(MessagePackage::key_MeetingID); // 将整数转换为字符串
    addMeetingList(meetingName,meetingId);
}

void UserMenu::onJoinMeeting(const MessagePackage &pack)
{
    int ret=pack.getIntValue(MessagePackage::Key_Result);
    qDebug()<<"join ret"<<ret;
    if(ret==1)
    {
        int meetingID=pack.getIntValue(MessagePackage::key_MeetingID);
        QString meetname=pack.getStringValue(MessagePackage::key_MeetingName);
        QString hostname=pack.getStringValue(MessagePackage::Key_Name);
        int messageport=pack.getIntValue(MessagePackage::Key_MessagePort);
        int videoport=pack.getIntValue(MessagePackage::Key_VideoPort);
        int mediaport=pack.getIntValue(MessagePackage::Key_MediaPort);
        addMeetingList(meetname,meetingID);
        qDebug()<<" join meetname: "<<meetname<<"ID: "<<meetingID;
        qDebug()<<ret<<"===="<<messageport<<"=="<<videoport<<"=="<<mediaport;
        if(!mr_w){
            timer=new QTimer(this);
            connect(timer,&QTimer::timeout,this,&UserMenu::requestMeetingMembers);
            timer->start(3000);//定时器，定时刷新会议成员列表

            mr_w=new MeetingRoom(meetname,username,hostname,meetingID,messageport,videoport,mediaport);
            connect(mr_w,&MeetingRoom::invited,this,&UserMenu::oninvited);
            connect(mr_w,&MeetingRoom::meetingRoomClose,this,&UserMenu::onMeetingRoomClose);
            mr_w->setWindowTitle(meetname);
            mr_w->show();
        }else{
            QMessageBox::warning(this,"参加会议","只能参加一次会议");
        }
    }
    else{

    }
}

//关闭会议窗口；退出会议
void UserMenu::onMeetingRoomClose()
{
    //服务器处理
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_CloseMeeting);//设置发送包类型
    if(username==mr_w->getHostName()){
        pack.addValue(MessagePackage::Key_Name,mr_w->getHostName());//会议主持人
    }
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::key_MeetingName,mr_w->getMeetingName());//会议名
    pack.addValue(MessagePackage::key_MeetingID,mr_w->getMeetingID());//会议id
    qDebug()<<"exit meetingId"<<mr_w->getMeetingID();
    pack.sendMsg(socket);
}
//主持人将会议关闭
void UserMenu::onMeetingClosed(const MessagePackage &pack)
{
    QString meetname=pack.getStringValue(MessagePackage::key_MeetingName);
    QString hostname=pack.getStringValue(MessagePackage::Key_Name);
    int meetingID = pack.getIntValue(MessagePackage::key_MeetingID);
    int ret=pack.getIntValue(MessagePackage::Key_Result);
    qDebug()<<"ret:"<<ret;
    if(ret==1){
        int rowCount = ui->tableWidget_meetingList->rowCount();
        for (int i = 0; i < rowCount; ++i) {
            // 获取会议ID和会议名称所在的单元格项
            QTableWidgetItem *idItem = ui->tableWidget_meetingList->item(i, 0);
            QTableWidgetItem *nameItem = ui->tableWidget_meetingList->item(i, 1);
            if (idItem && nameItem) {
                // 检查会议ID和会议名称是否匹配
                if (idItem->text() == QString::number(meetingID) && nameItem->text() == meetname) {
                    // 删除匹配的行
                    ui->tableWidget_meetingList->removeRow(i);
                    break; // 如果只需要找到第一个匹配项，找到后就可以退出循环
                }
            }
        }
        //关闭窗口
        if(mr_w){
            if(timer){
                timer->stop();
                timer->deleteLater();
                timer=nullptr;
            }
            mr_w->hide();
            mr_w->deleteLater();
            mr_w = nullptr; // 防止悬空指针
        }
        QMessageBox::information(this,"结束会议",hostname+"关闭了"+meetname);
    }else{
        QMessageBox::warning(this,"结束会议","会议结束失败，未知情况");
    }
}

void UserMenu::onMeetingExit(const MessagePackage &pack) {
    int ret = pack.getIntValue(MessagePackage::Key_CloseMeeting);
    qDebug()<<"close ret:"<<ret;
    if (ret == 2) {
        QString meetname = pack.getStringValue(MessagePackage::key_MeetingName);
        int meetingID = pack.getIntValue(MessagePackage::key_MeetingID);
        // 获取表格当前的行数
        int rowCount = ui->tableWidget_meetingList->rowCount();
        for (int i = 0; i < rowCount; ++i) {
            // 获取会议ID和会议名称所在的单元格项
            QTableWidgetItem *idItem = ui->tableWidget_meetingList->item(i, 0);
            QTableWidgetItem *nameItem = ui->tableWidget_meetingList->item(i, 1);
            if (idItem && nameItem) {
                // 检查会议ID和会议名称是否匹配
                if (idItem->text() == QString::number(meetingID) && nameItem->text() == meetname) {
                    // 删除匹配的行
                    ui->tableWidget_meetingList->removeRow(i);
                    break; // 如果只需要找到第一个匹配项，找到后就可以退出循环
                }
            }
        }
        if(mr_w!=nullptr){
            //关闭窗口
            if(timer){
                timer->stop();
                timer->deleteLater();
                timer=nullptr;
            }
            mr_w->hide();
            mr_w->deleteLater();
            mr_w = nullptr; // 防止悬空指针
        }
        QMessageBox::information(this,"退出会议","退出会议成功");
    }else{
        QMessageBox::information(this,"退出会议","退出会议失败");
    }
}

void UserMenu::onMeetingMembersList(const MessagePackage &pack)
{
    QStringList membersIn=pack.getListValue(MessagePackage::Key_MeetingMembersIn);
    QStringList membersAbsent=pack.getListValue(MessagePackage::Key_MeetingMembersAbsent);
    QStringList membersLeft=pack.getListValue(MessagePackage::Key_MeetingMembersLeave);
    if(mr_w){//如果会议还在开启状态
        mr_w->setMembers(membersIn,membersLeft,membersAbsent);
    }
}

void UserMenu::closeEvent(QCloseEvent *event)
{
    if (fileThrd) {
        fileThrd->quit(); // 结束线程事件循环
        fileThrd->wait(); // 等待线程结束
        fileThrd = nullptr;
        filehelper = nullptr;
    }
    emit userMenuClosed(); // 发出信号
    QMainWindow::closeEvent(event);
}
//双击列表项切换窗口
void UserMenu::list_UserList_itemDoubleClicked(QListWidgetItem *item) {
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
        ChatMenu* chat = addNewCharMenu(false);
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
//更新显示列表
void UserMenu::on_pb_refllushList_clicked()
{
    updateUsrListRequest();
}

//新建群组
void UserMenu::on_pb_newgroup_clicked() {
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


void UserMenu::on_pb_invite_clicked()
{
    // 获取群组列表中选中的项
    QListWidgetItem *groupItem = ui->list_groupList->currentItem();
    // 检查是否选择了群组和用户
    if (!groupItem) {
        //qDebug() << "没有选择群组或用户";
        QMessageBox::warning(this,"邀请群员","请选中群组");
        return; // 如果没有选择列表项，则返回
    }
    // 获取用户列表中选中的项
    QListWidgetItem *userItem = ui->list_UserList->currentItem();
    if (!userItem) {
        //qDebug() << "没有选择群组或用户";
        QMessageBox::warning(this,"邀请群员","请选中群员");
        return; // 如果没有选择列表项，则返回
    }
    // 创建消息包对象
    MessagePackage pack;
    QString groupName = groupItem->text(); // 获取群组名称
    QString objUserName = userItem->text(); // 获取用户名称

    // 设置消息类型和内容
    pack.setType(MessagePackage::Key_Type_InviteGroupMember);
    pack.addValue(MessagePackage::Key_GroupName, groupName);
    pack.addValue(MessagePackage::Key_Sender, username);
    pack.addValue(MessagePackage::Key_Name, objUserName);

    // 发送消息
    pack.sendMsg(socket);
}


void UserMenu::on_list_groupList_itemDoubleClicked(QListWidgetItem *item)
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
        ChatMenu* chat =addNewCharMenu(true); // 设置 ui->frame 作为父对象
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

//新建聊天窗口处理
ChatMenu *UserMenu::addNewCharMenu(bool group)
{
    ChatMenu* chat = new ChatMenu(ui->frame,group); // 设置 ui->frame 作为父对象
    connect(chat,&ChatMenu::sendPrivateChat,this,&UserMenu::getPrivateMsg);
    connect(chat,&ChatMenu::sendGroupChat,this,&UserMenu::getGroupMsg);
    connect(chat,&ChatMenu::fileSent,this,&UserMenu::onFileSent);
    connect(chat,&ChatMenu::flushGrroupMembers,this,&UserMenu::onFlushGroupMembers);
    connect(chat,&ChatMenu::flushFileList,this,&UserMenu::onFlushFileList);
    connect(chat,&ChatMenu::requestFile,this,&UserMenu::onRequestFile);
    return chat;
}

void UserMenu::addMeetingList(const QString &meetingName, const int &meetingID)
{
    // 获取表格当前的行数
    int rowCount = ui->tableWidget_meetingList->rowCount();
    bool exists = false;
    // 检查会议是否已存在
    for (int i = 0; i < rowCount; ++i) {
        QTableWidgetItem *idItem = ui->tableWidget_meetingList->item(i, 0);
        QTableWidgetItem *nameItem = ui->tableWidget_meetingList->item(i, 1);

        if (idItem && nameItem) {
            if (idItem->text() == QString::number(meetingID) && nameItem->text() == meetingName) {
                exists = true;
                break;
            }
        }
    }
    // 如果会议不存在，则添加会议信息到列表
    if (!exists) {
        ui->tableWidget_meetingList->insertRow(rowCount);
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(meetingID));
        QTableWidgetItem *nameItem = new QTableWidgetItem(meetingName);
        ui->tableWidget_meetingList->setItem(rowCount, 0, idItem); // 第一列是会议号
        ui->tableWidget_meetingList->setItem(rowCount, 1, nameItem); // 第二列是会议名称
    }
}


void UserMenu::on_pb_addmeet_clicked()
{
    if(mr_w!=nullptr){
        QMessageBox::warning(this,"新建会议","不能重复参加会议");
        return;
    }
    QString meetingName=CreateMeetingDialog::toCreateMeeting(username,this);
    qDebug()<<"新建会议:"<<meetingName;
    if(meetingName.isEmpty())
    {
        return;//取消建立
    }
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_CreateMeeting);
    pack.addValue(MessagePackage::Key_Name,username);
    pack.addValue(MessagePackage::key_MeetingName,meetingName);
    pack.sendMsg(socket);
}


void UserMenu::on_pb_joinmeet_clicked()
{
    if(mr_w!=nullptr){
        QMessageBox::warning(this,"新建会议","不能重复参加会议");
        return;
    }
    // 获取当前选中的行
    QTableWidget *tableWidget = ui->tableWidget_meetingList;
    QTableWidgetItem *currentItem = tableWidget->currentItem();

    if (currentItem) {
        // 获取会议号（假设会议号在第一列）
        QString meetingID = currentItem->text();
        // 获取会议名称（假设会议名称在第二列）
        int row = currentItem->row();
        QTableWidgetItem *meetingNameItem = tableWidget->item(row, 1);
        if (meetingNameItem) {
            QString meetingName = meetingNameItem->text();
            qDebug() << "Meeting ID:" << meetingID;
            qDebug() << "Meeting Name:" << meetingName;
            MessagePackage pack;
            pack.setType(MessagePackage::Key_Type_JoinMeeting); // 设置发送包类型
            pack.addValue(MessagePackage::Key_Sender, username); // 发送者
            pack.addValue(MessagePackage::key_MeetingName, meetingName); // 会议名
            pack.addValue(MessagePackage::key_MeetingID, meetingID.toInt()); // 会议id
            pack.sendMsg(socket);
        } else {
            QMessageBox::warning(this, "错误", "会议名称为空");
        }
    } else {
        QMessageBox::warning(this, "错误", "请选择一个会议");
    }
}

