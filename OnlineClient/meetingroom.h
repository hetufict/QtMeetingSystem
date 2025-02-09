#ifndef MEETINGROOM_H
#define MEETINGROOM_H

#include <QWidget>
#include <QUdpSocket>
#include <QCloseEvent>
#include <QImage>
#include "camerahandler.h"
#include "videorecover.h"
#include "videosender.h"
namespace Ui {
class MeetingRoom;
}

class MeetingRoom : public QWidget
{
    Q_OBJECT

public:
    explicit MeetingRoom(QString meetName,QString username,QString hostname,int meetingID,int mp,int vp,int mdp,QWidget *parent = nullptr);
    ~MeetingRoom();
    QString getHostName();//hostName接口
    QString getMeetingName();//meetingname接口
    int getMeetingID();//meetingID接口
    void setMembers(const QStringList& membersIn,const QStringList& membersLeft,const QStringList& membersAbsent);
    Ui::MeetingRoom *ui;
public slots:
    void onGetImage(QImage img);//设置摄像头捕获图片
    //void sendeVideo(QImage image);//按帧发送视频
    //void onVideoReadyRead();//接收图片
    void videoSet(QImage image);//收到视频帧
signals:
    void invited(const QString &name, const QString &meetName,int& meetingID);
    void meetingRoomClose(); // 发出信号让chatMune处理关闭事件
    void recvComplete(QImage image);//图片帧接收完成
    void stopVideoSender();//通知停止发送线程
    void stopVideoRecv();//通知停止接收线程
private slots:
    void onMsgReadyRead();//收到会议消息
    void on_pb_msgSend_clicked();//发送会议聊天
    void on_pb_invite_clicked();//发送会议邀请
    void on_CB_cremaSet_stateChanged(int arg1);//摄像头开关
    void on_CB_micSet_stateChanged(int arg1); // 麦克风

protected:
virtual void  closeEvent(QCloseEvent *event)override;//重写关闭事件，退出会议
private:
void videoRecoverInit();
void videoSenderInit();
private:
    CameraHandler* camera;
    QString meetname;//会议名称
    QString username;//用户名
    QString hostname;//会议主持人
    QUdpSocket *ms;//文字消息套接字
    //QUdpSocket *vs;//处理视频信息套接字
    QUdpSocket *mds;//音频信息套接字
    int meetingID;//会议ID
    int mp;//文字信息端口
    int vp;//视频信息端口
    int mdp;//音频信息端口
    VideoRecover* recover;//视频信息接受管理类
    VideoSender* sender;//视频信息发送管理类
    QThread* vrecvThread;//视频信息接受线程
    QThread* vsendThread;//视频信息发送线程
};

#endif // MEETINGROOM_H
