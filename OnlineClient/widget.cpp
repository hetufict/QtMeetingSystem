#include "widget.h"
#include "./ui_widget.h"
#include <QMessageBox>
#include <QDebug>
#include <QDir>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    ,usrMenu(nullptr)
{
    ui->setupUi(this);
    socket = new QTcpSocket(this);
    socket->connectToHost(QHostAddress("127.0.0.1"),8888);
    connect(socket,&QTcpSocket::readyRead,this,&Widget::onReadyRead);
}
void Widget::onReadyRead()
{
    while (socket->bytesAvailable() > 0) {
        // 读取协议包
        MessagePackage pack(nullptr, 0);
        pack.recvMsg(socket);
        // 处理协议包
        resultHandler(pack);
    }
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pb_login_clicked()
{
    QString name = ui->lineEdit_account->text();
    QString pswd = ui->lineEdit_password->text();
    if(name.isEmpty()||pswd.isEmpty())
    {
        QMessageBox::warning(this,"登录","用户名和密码不能为空");
        return;
    }
    //封装数据包
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_Login);
    pack.addValue("name",name);
    pack.addValue("pswd",pswd);
    username=name;
    //发送
    pack.sendMsg(socket);
    // qDebug()<<"name:"<<pack.getStringValue("name")<<" pswd:"<<pack.getStringValue("pswd");
}


void Widget::on_pb_register_clicked()
{
    QString name = ui->lineEdit_account->text();
    QString pswd = ui->lineEdit_password->text();
    if(name.isEmpty()||pswd.isEmpty())
    {
        QMessageBox::warning(this,"注册","用户名和密码不能为空");
        return;
    }
    //封装数据包
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_Register);
    pack.addValue("name",name);
    pack.addValue("pswd",pswd);
    //发送
    pack.sendMsg(socket);
}

void Widget::resultHandler(const MessagePackage &pack)
{
    //MessagePackage tempPack = pack; // 创建一个非常量的副本
    if(pack.Type()==MessagePackage::Key_Type_Login)
    {
        loginResultHandler(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_Register)
    {
        registerResultHandler(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_UpdateList)
    {
        emit updateListRespond(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_PrivateChat)
    {
        emit receivePrivateChat(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_AddGroup)
    {
        emit addGroupRet(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_GroupChat)
    {
        // qDebug()<<"get group chat pack";
        emit receiveGroupChat(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_GetGroupMembers)
    {
        emit receiveGroupMembers(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_FileOK)//发送文件的结果回复
    {
        if(pack.getStringValue(MessagePackage::Key_Sender)==usrMenu->getUsername())
        {
            emit sendFileRespond(pack);//发送私聊文件成功
        }
        else
        {
            emit recvPrivateFile(pack);//收到私聊文件
        }
    }
    else if(pack.Type()==MessagePackage::Key_Type_GetFileList)//私聊文件处理
    {
        emit receiveFileList(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_GetFile)//文件下载处理
    {
        emit receiveFile(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_CreateMeeting)
    {
        emit createMeeting(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_InviteMeeting)
    {
        if(pack.getStringValue(MessagePackage::Key_Sender)==usrMenu->getUsername())
        {
            emit inviteMeetingRespond(pack);//发送私聊文件成功
        }
        else
        {
            emit getMeetingInvitation(pack);//收到私聊文件
        }
    }
    else if(pack.Type()==MessagePackage::Key_Type_JoinMeeting)
    {
        emit joinMeeting(pack);
    }
    else if(pack.Type()==MessagePackage::Key_Type_CloseMeeting)
    {
        if(pack.getIntValue(MessagePackage::Key_Type_CleanMeeting)==1)
        {
            emit meetingClosed(pack);//结束会议
        }
        else
        {
            emit meetingExit(pack);//退出会议
        }
        qDebug()<<pack.getStringValue(MessagePackage::Key_Type_CleanMeeting);
    }else if(pack.Type()==MessagePackage::Key_Type_MembersList)
    {
        emit meetingMembersList(pack);
    }else if(pack.Type()==MessagePackage::Key_Type_UpdateLists){
        emit updateLists(pack);
    }

}

void Widget::loginResultHandler(const MessagePackage &pack)
{
    if(pack.getIntValue("result")==1)
    {
        QString username=pack.getStringValue("name");
        usrMenu=new UsrMenu(this,socket,username);
        connect(usrMenu, &UsrMenu::usrMenuClosed, this, &Widget::onUserMenuClosed);
        QMessageBox::information(this,"登陆成功","登陆成功");
        QDir dir("./fileData/"+username);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                qDebug() << "无法创建目录：" << dir.path();
                return;
            }
        }
        this->hide();
        usrMenu->show();
        //设置信号和槽处理用户请求
        adminConnectUserMenu();
    }
    else if(pack.getIntValue("result")==2)
    {
        QMessageBox::warning(this,"登陆失败","登陆失败，账号密码重复，不能重复登陆");
    }
    else if(pack.getIntValue("result")==0)
    {
        QMessageBox::warning(this,"登陆失败","登陆失败，账号密码错误");
    }
    else
    {
        QMessageBox::warning(this,"登陆失败","502");
    }
}

void Widget::registerResultHandler(const MessagePackage &pack)
{
    if(pack.getIntValue("result"))
    {
        QMessageBox::information(this,"注册","注册成功");
    }
    else
    {
        QMessageBox::warning(this,"注册失败","注册失败，账号已存在");
    }
}

void Widget::adminConnectUserMenu()
{
    connect(this,&Widget::updateListRespond,usrMenu->getChatObj(),&ChatInfo::updateUserList);
    connect(this,&Widget::receivePrivateChat,usrMenu->getChatObj(),&ChatInfo::onReceivePrivateChat);
    connect(this,&Widget::addGroupRet,usrMenu->getChatObj(),&ChatInfo::onAddaddGroupRet);
    connect(this,&Widget::receiveGroupChat,usrMenu->getChatObj(),&ChatInfo::onReceiveGroupChat);
    connect(this,&Widget::receiveGroupMembers,usrMenu->getChatObj(),&ChatInfo::onReceiveGroupMembers);
    connect(this,&Widget::sendFileRespond,usrMenu->getChatObj(),&ChatInfo::onsendFileRespond);
    connect(this,&Widget::recvPrivateFile,usrMenu->getChatObj(),&ChatInfo::onrecvPrivateFile);
    connect(this,&Widget::receiveFileList,usrMenu->getChatObj(),&ChatInfo::onReceiveFileList);
    connect(this,&Widget::receiveFile,usrMenu->getChatObj(),&ChatInfo::onReceiveFile);
    connect(this,&Widget::updateLists,usrMenu->getChatObj(),&ChatInfo::onUpdateLists);


    connect(this,&Widget::createMeeting,usrMenu->getMeetingObj(),&MeetiingMenu::onCreateMeeting);
    connect(this,&Widget::inviteMeetingRespond,usrMenu->getMeetingObj(),&MeetiingMenu::onInviteMeetingRespond);
    connect(this,&Widget::getMeetingInvitation,usrMenu->getMeetingObj(),&MeetiingMenu::onGetMeetingInvitation);
    connect(this,&Widget::joinMeeting,usrMenu->getMeetingObj(),&MeetiingMenu::onJoinMeeting);
    connect(this,&Widget::meetingClosed,usrMenu->getMeetingObj(),&MeetiingMenu::onMeetingClosed);
    connect(this,&Widget::meetingExit,usrMenu->getMeetingObj(),&MeetiingMenu::onMeetingExit);
    connect(this,&Widget::meetingMembersList,usrMenu->getMeetingObj(),&MeetiingMenu::onMeetingMembersList);
}

void Widget::onUserMenuClosed() {
    this->show(); // 显示 Widget 窗口
    //封装数据包
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_Logout);
    pack.addValue(MessagePackage::Key_Name,username);
    //发送
    pack.sendMsg(socket);
    if (usrMenu) {
        //usrMenu->deleteLater(); // 如果需要，删除 usrMenu 对象(已经指定父对象)
    }
}

