#ifndef VIDEORECOVER_H
#define VIDEORECOVER_H

#include <QObject>
#include <QUdpSocket>
#include <QImage>
#include <QThread>
typedef struct
{
    int width; //宽度
    int height; //高度
    int bytesPerline; //每行字节数
    bool isLastPack;
    int packTaken;
    char sender[64];
    char data[1024];
}ImagePackage;
class VideoRecover : public QObject
{
    Q_OBJECT
public:
    explicit VideoRecover(int port,QObject *parent = nullptr);
private:
    QUdpSocket* socket;
    int port;
    bool isSpy=true;
public slots:
    void onReadyRead();
    void onStopVideoRecv();
signals:
    void recvComplete(QImage image);
};

#endif // VIDEORECOVER_H
