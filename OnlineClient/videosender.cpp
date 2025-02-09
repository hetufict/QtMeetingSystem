#include "videosender.h"
#include <QVariant>
struct ImagePackage {
    int width;          // 宽度
    int height;         // 高度
    int bytesPerline;   // 每行字节数
    bool isLastPack;    // 是否是最后一个数据包
    int packTaken;      // 数据包计数
    char sender[64];     // 发送者
    char data[1024];     // 数据
};

VideoSender::VideoSender(int port,QObject *parent)
    : QObject{parent},port(port)
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

void VideoSender::sendeVideo(QImage image)
{
    //static int i=0;
    int unitBytes = 1024;
    int byteCount = image.sizeInBytes(); //图片大小
    int width = image.width();
    int height = image.height();
    int bytePerLine = image.bytesPerLine();
    //qDebug()<<"bytesCount:"<<byteCount<<" width&height"<<width<<","<<height<<"bytePerLine:"<<bytePerLine;
    int writeBytes = 0; //已写入大小

    while(true)
    {
        ImagePackage pack;
        memset(&pack,0,sizeof(ImagePackage));
        pack.width = width;
        pack.height = height;
        pack.bytesPerline = bytePerLine;
        if(writeBytes<byteCount)
        {
            memcpy(pack.data,((char*)image.bits())+writeBytes,unitBytes);

            writeBytes += unitBytes;
            if(writeBytes>=byteCount)
            {
                pack.isLastPack = true;
                pack.packTaken = unitBytes-(writeBytes-byteCount); //获取当前包实际的数据大小
            }
            else
            {
                pack.isLastPack = false;
                pack.packTaken = unitBytes; //设置当前包中数据的大小
            }
            //qDebug()<<image.size()<<"writeBytes:"<<writeBytes<<"taken"<<pack.packTaken<<"count"<<byteCount;
            socket->writeDatagram((char*)&pack,sizeof(ImagePackage),QHostAddress("224.0.1.0"),port);
        }
        else
        {
            //if(++i%30==0)
                //qDebug()<<i<<image.size()<<"writeBytes:"<<writeBytes<<"taken"<<pack.packTaken<<"count"<<byteCount;
            break;
        }
    }
}

void VideoSender::onStopVideoSender()
{
    socket->abort();
    QThread::currentThread()->quit();
    //QMetaObject::invokeMethod(vrecvThread, "quit", Qt::QueuedConnection);
}
