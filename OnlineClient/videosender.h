#ifndef VIDEOSENDER_H
#define VIDEOSENDER_H

#include <QObject>
#include <QUdpSocket>
#include <QImage>
#include <QThread>
class VideoSender : public QObject
{
    Q_OBJECT
public:
    explicit VideoSender(int port,QObject *parent = nullptr);
signals:
public slots:
    void sendeVideo(QImage image);
    void onStopVideoSender();
private:
    QUdpSocket* socket;
    int port;
};

#endif // VIDEOSENDER_H
