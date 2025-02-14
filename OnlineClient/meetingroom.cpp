#include "meetingroom.h"
#include "ui_meetingroom.h"
#include <QDebug>
#include <QThread>
MeetingRoom::MeetingRoom(QString meetName,QString username,QString hostname,int meetingID,int mp,int vp,int mdp,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MeetingRoom)
    ,camera(nullptr),sender(nullptr),vsendThread(nullptr),meetname(meetName),username(username)
    ,hostname(hostname),meetingID(meetingID),mp(mp),vp(vp),mdp(mdp)
{
    ui->setupUi(this);

    ms=new QUdpSocket(this);
    //vs=new QUdpSocket(this);
    mds=new QUdpSocket(this);
    ms->bind(QHostAddress::AnyIPv4,mp,QAbstractSocket::ReuseAddressHint);
    mds->bind(QHostAddress::AnyIPv4,mdp,QAbstractSocket::ReuseAddressHint);

    bool ret=ms->joinMulticastGroup(QHostAddress("224.0.1.0"));
    if(!ret){
        qDebug()<<ms->error();
    }
    videoRecoverInit();//准备接受视频线程
    mds->joinMulticastGroup(QHostAddress("224.0.1.0"));
    connect(ms,&QUdpSocket::readyRead,this,&MeetingRoom::onMsgReadyRead);
}

MeetingRoom::~MeetingRoom()
{
    delete ui;
    // 停止发送线程
    if (vsendThread) {
        //qDebug() << "send thread stop";
        if(vsendThread->isRunning()){
            emit stopVideoSender();
            vsendThread->wait();
        }
        vsendThread->deleteLater();
        vsendThread=nullptr;
        sender->deleteLater();
        sender = nullptr;
    }
    // 停止接收线程
    if (vrecvThread) {
        qDebug() << "recv thread stop";
        if(vrecvThread->isRunning()){

            qDebug() << "call recv thread stop";
            emit stopVideoRecv();
            vrecvThread->wait();
        }
        delete sender;
        sender=nullptr;
        vrecvThread->deleteLater();
        vrecvThread=nullptr;
    }
    //关闭相机
    if (camera) {
        delete camera;
        camera = nullptr;
    }
}

QString MeetingRoom::getHostName()
{
    return hostname;
}

QString MeetingRoom::getMeetingName()
{
    return meetname;
}

int MeetingRoom::getMeetingID()
{
    return meetingID;
}

void MeetingRoom::setMembers(const QStringList &membersIn, const QStringList &membersLeft, const QStringList &membersAbsent) {
    ui->listWidget_members->clear();

    // 设置成员在会议中的背景色
    QColor colorIn = QColor(0, 255, 0); // 绿色
    // 设置成员离开会议的背景色
    QColor colorLeft = QColor(255, 165, 0); // 橙色
    // 设置成员缺席的背景色
    QColor colorAbsent = QColor(255, 0, 0); // 红色

    // 添加成员在会议中的列表项
    for (const QString &member : membersIn) {
        QListWidgetItem *item = new QListWidgetItem(member);
        item->setBackground(QBrush(colorIn));
        ui->listWidget_members->addItem(item);
    }

    // 添加成员离开会议的列表项
    for (const QString &member : membersLeft) {
        QListWidgetItem *item = new QListWidgetItem(member);
        item->setBackground(QBrush(colorLeft));
        ui->listWidget_members->addItem(item);
    }

    // 添加成员缺席的列表项
    for (const QString &member : membersAbsent) {
        QListWidgetItem *item = new QListWidgetItem(member);
        item->setBackground(QBrush(colorAbsent));
        ui->listWidget_members->addItem(item);
    }
}

void MeetingRoom::onGetImage(QImage img)
{
    ui->label->setPixmap(QPixmap::fromImage(img));
}


void MeetingRoom::videoSet(QImage image)
{
    image = image.scaled(ui->label->width(),ui->label->height(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    QPixmap picture = QPixmap::fromImage(image); //从QImage图片对象转成QPixmap格式图片对象
    qDebug()<<image.size()<<picture.size();
    ui->label->setPixmap(QPixmap::fromImage(image));
}

void MeetingRoom::onMsgReadyRead()
{
    // int len=ms->pendingDatagramSize();
    // char msg[len];
    // memset(msg,0,len);
    // ms->readDatagram(msg,len);

    // ui->textBrowser_msg->append(msg);
    QByteArray datagram;
    datagram.resize(ms->pendingDatagramSize());
    ms->readDatagram(datagram.data(), datagram.size());
    ui->textBrowser_msg->append(QString::fromUtf8(datagram));
}

void MeetingRoom::on_pb_msgSend_clicked()
{
    QString msg=ui->lineEdit_msg->text();
    if(msg.isEmpty())
    {
        return;
    }
    msg=username+": "+msg;
    // 使用 c_str() 获取 const char* 指针
    ms->writeDatagram(msg.toStdString().c_str(), strlen(msg.toStdString().c_str()) + 1,
                      QHostAddress("224.0.1.0"), mp);
}


void MeetingRoom::on_pb_invite_clicked()
{
    QString name=ui->lineEdit_invite->text();
    if(name.isEmpty())
    {
        return;
    }
    emit invited(name,meetname,meetingID);
}

void MeetingRoom::closeEvent(QCloseEvent *event)
{
    //qDebug()<<"close room";
    emit meetingRoomClose(); // 发出信号
    event->ignore(); // 阻止窗口关闭的默认行为
}

void MeetingRoom::videoRecoverInit()
{
    // vs->bind(QHostAddress::AnyIPv4,vp,QAbstractSocket::ReuseAddressHint);
    // vs->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,300*2048);//设置udp缓冲区
    // bool ret=vs->joinMulticastGroup(QHostAddress("224.0.1.0"));//组播地址
    // if(!ret){
    //     qDebug()<<vs->error();
    // }
    recover = new VideoRecover(vp);
    vrecvThread = new QThread(this);
    recover ->moveToThread(vrecvThread);
    //vs->moveToThread(vrecvThread);
    connect(vrecvThread,&QThread::started,recover,&VideoRecover::onReadyRead);
    connect(recover ,&VideoRecover::recvComplete,this,&MeetingRoom::videoSet,Qt::BlockingQueuedConnection);
    connect(this,&MeetingRoom::stopVideoRecv,recover,&VideoRecover::onStopVideoRecv);
    vrecvThread->start();
}

void MeetingRoom::videoSenderInit()
{
    // vs->bind(QHostAddress::AnyIPv4,vp,QAbstractSocket::ReuseAddressHint);
    // vs->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,300*2048);//设置udp缓冲区
    // bool ret=vs->joinMulticastGroup(QHostAddress("224.0.1.0"));//组播地址
    // if(!ret){
    //     qDebug()<<vs->error();.
    //  }
    sender = new VideoSender(vp);
    vsendThread = new QThread(this);
    sender->moveToThread(vsendThread);
    //vs->moveToThread(vrecvThread);
    connect(camera, &CameraHandler::getImage, sender, &VideoSender::sendeVideo, Qt::BlockingQueuedConnection);
    connect(this,&MeetingRoom::stopVideoSender,sender,&VideoSender::onStopVideoSender);
    vsendThread->start();
}


void MeetingRoom::on_CB_cremaSet_stateChanged(int arg1) {
    if (arg1 == Qt::Checked) {
        // 停止接收线程
        if (vrecvThread) {
            qDebug() << "recv thread stop";
            if(vrecvThread->isRunning()){

                qDebug() << "call recv thread stop";
                emit stopVideoRecv();
                vrecvThread->wait();
            }
            delete recover;
            recover=nullptr;
            vrecvThread->deleteLater();
            vrecvThread=nullptr;
        }
        // 创建摄像头对象，开启视频，发送视频数据
        if (!camera) {
            camera = new CameraHandler(this);
            connect(camera, &CameraHandler::getImage, this, &MeetingRoom::onGetImage);
            if (!sender) { // 初始化
                qDebug() << "send thread start";
                videoSenderInit();
            }
        }
    } else {
        // 停止发送线程
        if (vsendThread) {
            qDebug() << "send thread stop";
            if(vsendThread->isRunning()){
                emit stopVideoSender();
                vsendThread->wait();
            }
            vsendThread->deleteLater();
            vsendThread=nullptr;
            sender->deleteLater();
            sender = nullptr;
        }
        // 开启接收线程
        if (!vrecvThread) {
            qDebug() << "recv thread start";
            videoRecoverInit();
        }
        delete camera;
        camera = nullptr;
    }
}

void MeetingRoom::on_CB_micSet_stateChanged(int arg1)
{
    //开启麦克风
    if (arg1 == Qt::Checked) {

    }
    else//关闭麦克风
    {

    }
}

