#include <QMessageBox>
#include "meetiingmenu.h"
#include "ui_meetiingmenu.h"
#include "createmeetingdialog.h"
#include "logger.h"
MeetiingMenu::MeetiingMenu(QWidget *parent,QTcpSocket* socket,QString username)
    : QWidget(parent)
    , ui(new Ui::MeetiingMenu)
    ,socket(socket)
    ,username(username)
    ,mr_w(nullptr)
    ,timer(nullptr)
{
    ui->setupUi(this);
}

MeetiingMenu::~MeetiingMenu()
{
    delete ui;
}

void MeetiingMenu::requestMeetingMembers()
{
    if(mr_w){//如果会议开启则发送请求
        MessagePackage pack;
        pack.setType(MessagePackage::Key_Type_MembersList);//设置请求类型
        pack.addValue(MessagePackage::Key_Name,username);//用户名
        pack.addValue(MessagePackage::key_MeetingID,mr_w->getMeetingID());//会议ID
        pack.addValue(MessagePackage::key_MeetingName,mr_w->getMeetingName());//会议名
        pack.sendMsg(socket);
    }
    LOG(Logger::Info,"request meemting members");
}

void MeetiingMenu::onCreateMeeting(const MessagePackage &pack)
{
    int ret=pack.getIntValue(MessagePackage::key_MeetingID);
    if(ret!=-1)
    {
        QString meetname=pack.getStringValue(MessagePackage::key_MeetingName);//会议主题
        QString hostname=pack.getStringValue(MessagePackage::Key_Name);//主持人
        int messageport=pack.getIntValue(MessagePackage::Key_MessagePort);//消息端口
        int videoport=pack.getIntValue(MessagePackage::Key_VideoPort);//视频端口
        int mediaport=pack.getIntValue(MessagePackage::Key_MediaPort);//音频端口
        addMeetingList(meetname,ret);
        if(!mr_w){
            timer=new QTimer(this);
            connect(timer,&QTimer::timeout,this,&MeetiingMenu::requestMeetingMembers);
            timer->start(3000);//定时器，定时刷新会议成员列表
            //初始化会议菜单
            mr_w=new MeetingRoom(meetname,username,hostname,ret,messageport,videoport,mediaport);
            connect(mr_w,&MeetingRoom::invited,this,&MeetiingMenu::oninvited);
            connect(mr_w,&MeetingRoom::meetingRoomClose,this,&MeetiingMenu::onMeetingRoomClose);
            mr_w->setWindowTitle(meetname);
            mr_w->show();
        }
        LOG(Logger::Info,"create a meeting: "+meetname);
    }
    else{

    }
}

void MeetiingMenu::oninvited(const QString &name, const QString &meetName, int &meetingID)
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_InviteMeeting);//设置发送包类型
    pack.addValue(MessagePackage::Key_Sender,username);//发送者
    pack.addValue(MessagePackage::Key_Receiver,name);//邀请的对象
    pack.addValue(MessagePackage::key_MeetingName,meetName);//会议名
    pack.addValue(MessagePackage::key_MeetingID,meetingID);//会议id
    pack.sendMsg(socket);
    LOG(Logger::Info,"send a meeting invitation");
}

void MeetiingMenu::onInviteMeetingRespond(const MessagePackage &pack)
{
    int ret=pack.getIntValue(MessagePackage::Key_Result);
    if(ret){
        QMessageBox::information(mr_w,"邀请会议成员","邀请成功");
        LOG(Logger::Info,"send a meeting invitation success");
    }else{
        QMessageBox::warning(mr_w,"邀请会议成员","未找到该成员");
        LOG(Logger::Info,"send a meeting invitation failed");
    }
}

void MeetiingMenu::onGetMeetingInvitation(const MessagePackage &pack)
{
    QString meetingName = pack.getStringValue(MessagePackage::key_MeetingName);
    int meetingId = pack.getIntValue(MessagePackage::key_MeetingID); // 将整数转换为字符串
    addMeetingList(meetingName,meetingId);
    LOG(Logger::Info,"receive a meeting invitation");
}

void MeetiingMenu::onJoinMeeting(const MessagePackage &pack)
{
    int ret=pack.getIntValue(MessagePackage::Key_Result);
    if(ret==1)
    {
        int meetingID=pack.getIntValue(MessagePackage::key_MeetingID);
        QString meetname=pack.getStringValue(MessagePackage::key_MeetingName);
        QString hostname=pack.getStringValue(MessagePackage::Key_Name);
        int messageport=pack.getIntValue(MessagePackage::Key_MessagePort);
        int videoport=pack.getIntValue(MessagePackage::Key_VideoPort);
        int mediaport=pack.getIntValue(MessagePackage::Key_MediaPort);
        addMeetingList(meetname,meetingID);
        if(!mr_w){
            timer=new QTimer(this);
            connect(timer,&QTimer::timeout,this,&MeetiingMenu::requestMeetingMembers);
            timer->start(3000);//定时器，定时刷新会议成员列表

            mr_w=new MeetingRoom(meetname,username,hostname,meetingID,messageport,videoport,mediaport);
            connect(mr_w,&MeetingRoom::invited,this,&MeetiingMenu::oninvited);
            connect(mr_w,&MeetingRoom::meetingRoomClose,this,&MeetiingMenu::onMeetingRoomClose);
            mr_w->setWindowTitle(meetname);
            mr_w->show();
            LOG(Logger::Info,"join a meeting invitation");
        }else{
            QMessageBox::warning(this,"参加会议","只能参加一场会议");
        }
    }
    else{

    }
}

void MeetiingMenu::onMeetingRoomClose()
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
    LOG(Logger::Info,"close meeting room");
    pack.sendMsg(socket);
}

void MeetiingMenu::onMeetingClosed(const MessagePackage &pack)
{
    QString meetname=pack.getStringValue(MessagePackage::key_MeetingName);
    QString hostname=pack.getStringValue(MessagePackage::Key_Name);
    int meetingID = pack.getIntValue(MessagePackage::key_MeetingID);
    int ret=pack.getIntValue(MessagePackage::Key_Result);
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
        LOG(Logger::Info,"meeting room closed");
    }else{
        QMessageBox::warning(this,"结束会议","会议结束失败，未知情况");
        LOG(Logger::Error,"meeting room closed unkown error");
    }
}

void MeetiingMenu::onMeetingExit(const MessagePackage &pack)
{
    int ret = pack.getIntValue(MessagePackage::Key_CloseMeeting);
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
        LOG(Logger::Info,"exit meeting room");
    }else{
        QMessageBox::information(this,"退出会议","退出会议失败");
        LOG(Logger::Info,"exit meeting room failed");
    }
}

void MeetiingMenu::onMeetingMembersList(const MessagePackage &pack)
{
    QStringList membersIn=pack.getListValue(MessagePackage::Key_MeetingMembersIn);
    QStringList membersAbsent=pack.getListValue(MessagePackage::Key_MeetingMembersAbsent);
    QStringList membersLeft=pack.getListValue(MessagePackage::Key_MeetingMembersLeave);
    if(mr_w){//如果会议还在开启状态
        mr_w->setMembers(membersIn,membersLeft,membersAbsent);
    }
    LOG(Logger::Info,"receive meeting members list");
}

void MeetiingMenu::on_pb_addmeet_clicked()
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
    LOG(Logger::Info,"requset a meeting room");
}

void MeetiingMenu::on_pb_joinmeet_clicked()
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

void MeetiingMenu::addMeetingList(const QString &meetingName, const int &meetingID)
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
