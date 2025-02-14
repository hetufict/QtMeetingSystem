#include "videorecover.h"
#include "QCoreApplication"
#include <QVariant>
VideoRecover::VideoRecover(int port,QObject *parent)
    : QObject{parent},socket(socket),port(port)
{
    socket=new QUdpSocket(this);
    socket->bind(QHostAddress::AnyIPv4,port,QAbstractSocket::ReuseAddressHint);
    //socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,300*2048);//设置udp缓冲区
    socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, QVariant(300 * 2048));
    bool ret=socket->joinMulticastGroup(QHostAddress("224.0.1.0"));//组播地址
    if(!ret){
        qDebug()<<socket->error();
    }
}
void VideoRecover::onReadyRead()
{
    //static int i=0;
    qDebug()<<1;
    QByteArray *imageBytes = new QByteArray(4000000,0);
    QDataStream *dataStream = new QDataStream(imageBytes,QIODevice::ReadWrite);
    while(isSpy)
    {
        if(socket->hasPendingDatagrams())
        {
            ImagePackage pack = {};
            memset(&pack,0,sizeof(ImagePackage));
            socket->readDatagram((char*)&pack,sizeof(ImagePackage));
            if(true == pack.isLastPack)
            {
                // qDebug()<<pack.packTaken<<pack.isLastPack;
                dataStream->writeRawData(pack.data,pack.packTaken);
                QImage image = QImage((uchar*)imageBytes->data(),
                                      pack.width,
                                      pack.height,
                                      pack.bytesPerline,
                                      QImage::Format_RGB888);

                emit recvComplete(image);
                dataStream->device()->seek(0);
            }
            else
            {
                //if(++i%30==0) qDebug()<<"recover:"<<i;
                dataStream->writeRawData(pack.data,pack.packTaken);
            }
        }
        // 检查是否有待处理的事件
        QCoreApplication::processEvents();
    }
    delete imageBytes;
    delete dataStream;
}
void VideoRecover::onStopVideoRecv()
{
    isSpy=false;
    //qDebug()<<"yes sir";
    //QMetaObject::invokeMethod(vrecvThread, "quit", Qt::QueuedConnection);
    socket->abort();
    QThread::currentThread()->quit();
}
