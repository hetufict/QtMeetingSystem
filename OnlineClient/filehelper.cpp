#include "filehelper.h"
#include <QFile>
#include <QDebug>
#include "messagepackage.h"
struct currentFileDataStream  {
    QString fileName;      // 文件名
    qint64 size;          // 文件总大小
    qint64 readSoFar;     // 已读取的数据大小
    QByteArray fileData;  // 接收到的文件数据
    QString reciever;//接受者
    // 重置结构体，以便开始接收新的文件数据
    void reset() {
        fileName.clear();
        size = 0;
        readSoFar = 0;
        fileData.clear();
        reciever.clear();
    }
};

FileHelper::FileHelper(bool send,bool group,QString filePath,QString fileName,QString sender,QString recver,QObject *parent)
    : QObject{parent},send(send),filePath(filePath),fileName(fileName),sender(sender),recver(recver),group(group)
{
    f_socket = new QTcpSocket(this);
    //connect(f_socket, &QTcpSocket::connected, this, &FileHelper::onConnected);
    connect(f_socket, &QTcpSocket::errorOccurred, this, &FileHelper::onError);
    f_socket->connectToHost(QHostAddress("127.0.0.1"),8888);
    //f_socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, QVariant(100 * 2048));
    //connect(socket,&QTcpSocket::readyRead,this,&Widget::onReadyRead);
    // 在 FileHelper 构造函数中连接 bytesWritten 信号
}

// 定义 onBytesWritten 槽函数
void FileHelper::onBytesWritten(qint64 bytes)
{
    qDebug() << "已发送" << bytes << "字节，剩余" << f_socket->bytesToWrite() << "字节在缓冲区";

    // 如果缓冲区中的字节数小于阈值，继续发送数据
    if (f_socket->bytesToWrite() < MAX_BUFFER_SIZE && !pause) {
        //sendNextChunk();
    }
}

void FileHelper::fileDeal()
{
    if(send){
        fileSend();
    }else{
        fileRecv();
    }
}

void FileHelper::fileSend()
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug()<<"无法打开文件"<<filePath<<" "<<file.errorString();
        return ;
    }
    qint64 filesize=file.size();
    QString logFileName=fileName+"Log.txt";
    QFile log(logFileName);
    if(!log.open(QIODevice::Append|QIODevice::Text)){
        qDebug()<<"无法打开日志文件"+log.errorString();
        file.close();
        return ;
    }
    QTextStream logStream(&log);
    logStream<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")<<" - 开始发送文件"<<fileName<<"\n";

    //发送文件
    qint64 fileSize=file.size();
    qint64 sentSize=0;
    //char buffer[4096];
    //qint64 bytesLeft=fileSize;
    const int bufferSize=1024;
    QByteArray buffer;
    while(fileSize-sentSize>0&&!pause){
        if(pause){
            logStream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - 文件发送已暂停\n";
            log.flush();  // 立即写入日志
            QThread::msleep(500);  // 每500毫秒检查一次暂停状态
            continue;
        }
        // 读取分块数据
        buffer = file.read(bufferSize);
        if (buffer.isEmpty()) {
            qDebug() << "文件读取错误";
            break;
        }
        //发送数据
        //QThread::msleep(50);
        MessagePackage pack;
        pack.setType(MessagePackage::Key_Type_FILEDATA);
        pack.addValue(MessagePackage::key_FileData,buffer);
        pack.addValue(MessagePackage::key_FileName,fileName);
        pack.addValue(MessagePackage::Key_Sender,sender);
        pack.addValue(MessagePackage::Key_Receiver,recver);
        pack.addValue(MessagePackage::Key_Result,group);
        pack.addValue(MessagePackage::Key_SentSize,buffer.size());
        pack.addValue(MessagePackage::key_FileSize,filesize);
        pack.sendMsg(f_socket);
        sentSize+=buffer.size();
        // 记录进度
        logStream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
                  << " - 已发送 " << sentSize << " 字节，剩余 " << fileSize-sentSize << " 字节\n";
        log.flush();  // 立即写入日志
    }
    file.close();
    log.close();
}

void FileHelper::fileRecv()
{

}
