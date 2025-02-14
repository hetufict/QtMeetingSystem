#include "filehelper.h"
#include <QFile>
#include <QDebug>
#include "messagepackage.h"

FileHelper::FileHelper(bool send, bool group, QString filePath, QString fileName, QString sender, QString recver, QObject *parent)
    : QObject{parent}, send(send), filePath(filePath), fileName(fileName), sender(sender), recver(recver), group(group), sentSize(0)
{
    //打开文件
    if(send)
    {
        file=new QFile(filePath);
        if(!file->open(QIODevice::ReadOnly)){
            qDebug()<<"无法打开文件"<<filePath<<" "<<file->errorString();
            return ;
        }
    }else{
        filePath=filePath+fileName;
        file=new QFile(filePath);
        if(!file->open(QIODevice::WriteOnly)){
            qDebug()<<"无法打开文件"<<filePath<<" "<<file->errorString();
            return ;
        }
    }
    qDebug()<<"file Size:"<<fileSize;
    //打开日志文件 记录发送情况
    QString logFileName=fileName+"Log.txt";
    log=new QFile(logFileName);
    if(!log->open(QIODevice::WriteOnly)){
        qDebug()<<"无法打开日志文件"+log->errorString();
        file->close();
    }
}

FileHelper::~FileHelper()
{
    qDebug()<<"XXXXXXXXXXXXXXfilehelp overXXXXXXXXXXXXX";
    if(file->isOpen()) file->close();
    if(log->isOpen())  log->close();
    //关闭套接字
    f_socket->close();
}

void FileHelper::setFileSize(const int &fileSize)
{
    this->fileSize=fileSize;
}


void FileHelper::onReadyRead()
{
    while (f_socket->bytesAvailable() > 0) {
        // 读取协议包
        MessagePackage pack(nullptr, 0);
        pack.recvMsg(f_socket);
        // 处理协议包
        resultHandler(pack);
    }
}

void FileHelper::resultHandler(MessagePackage &pack)
{
    if(pack.Type()==MessagePackage::Key_Type_FilePos){//收到接受确认
        //QThread::msleep(5);
        fileSend();//发送下一个包
    }else if(pack.Type()==MessagePackage::Key_Type_FileOK){
        emit fileSendFinished();
    }else if(pack.Type()==MessagePackage::Key_Type_FILEDATA){
        fileDownload(pack);
    }
}

void FileHelper::fileDeal()
{
    initSocket(); // 初始化 Socket
    //QThread::msleep(500);
    if(send){
        fileSend();
    }else{
        fileRecv();
    }
}

void FileHelper::fileSend()
{
    //发送文件
    qint64 totalSize=file->size();
    const int bufferSize=8192;
    // 检查套接字状态
    if (f_socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Socket error:" << f_socket->error();
        qDebug() << "Socket error string:" << f_socket->errorString();
        if(file->isOpen()) file->close();
        if(log->isOpen())  log->close();
        return;
    }
    // 定位到当前文件指针位置
    if (!file->seek(sentSize)) {
        qDebug() << "无法定位到文件位置" << sentSize << "错误：" << file->errorString();
        if(file->isOpen()) file->close();
        if(log->isOpen())  log->close();
        return;
    }
    // 读取分块数据
    QByteArray buffer;
    int currentSize=std::min(bufferSize,(int)(totalSize-sentSize));
    buffer = file->read(currentSize);
    if (buffer.isEmpty()) {
        qDebug() << "文件读取错误";
        if(file->isOpen()) file->close();
        if(log->isOpen())  log->close();
        return;
    }
    sentSize+=currentSize;
    //发送数据
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_FILEDATA);//包类型
    pack.addValue(MessagePackage::key_FileData,buffer);//文件数据
    pack.addValue(MessagePackage::key_FileName,fileName);//文件名
    pack.addValue(MessagePackage::Key_Sender,sender);//发送者
    pack.addValue(MessagePackage::Key_Receiver,recver);//接受者
    pack.addValue(MessagePackage::Key_Result,group);//是否是群文件
    pack.addValue(MessagePackage::Key_SentSize,sentSize);//已经发送的文件大小
    pack.addValue(MessagePackage::key_FileSize,totalSize);//文件的总大小
    pack.sendMsg(f_socket);
    qDebug()<<"buffer size:"<<buffer.size()<<"sent size: "<<sentSize<<" pack type:"<<pack.Type()<<" "<<f_socket->socketDescriptor();
    //qDebug()<<"buffer contain:\n"<<buffer.toStdString();
    //qDebug()<<"send by: "<<f_socket->socketDescriptor();
}
//请求文件数据
void FileHelper::fileRecv()
{
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_FileDataRequest);//包类型
    pack.addValue(MessagePackage::key_FileName,fileName);//文件名
    pack.addValue(MessagePackage::Key_Sender,sender);//发送者
    pack.addValue(MessagePackage::Key_Receiver,recver);//接受者
    pack.addValue(MessagePackage::Key_Result,group);//是否是群文件
    pack.addValue(MessagePackage::Key_SentSize,sentSize);//已经发送的文件大小
    pack.addValue(MessagePackage::key_FileSize,fileSize);//文件的总大小
    pack.sendMsg(f_socket);

}

void FileHelper::fileDownload(MessagePackage &pack)
{
    qDebug()<<"read file size:"<<pack.getIntValue(MessagePackage::Key_SentSize);
    //设置文件存储路径
    QByteArray fileData=pack.getFileValue(MessagePackage::key_FileData);
    //qint64 filesize=pack.getIntValue(MessagePackage::key_FileSize);
    sentSize=pack.getIntValue(MessagePackage::Key_SentSize);
    // 写入数据
    file->seek(sentSize-fileData.size()); // 定位到文件的上次写入的位置
    file->write(fileData);   // 写入数据
    // 记录文件已接受大小，回复接受数据确认
    if(sentSize < fileSize){
        fileRecv();//继续请求文件数据
    }
    else if (sentSize== fileSize){//如果文件上传完成
        emit fileSendFinished();
    }
}

void FileHelper::initSocket() {
    f_socket = new QTcpSocket(this);

    // 连接信号
    connect(f_socket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "Connected to server, socket descriptor:" << f_socket->socketDescriptor();
    });
    connect(f_socket, &QTcpSocket::errorOccurred, this, &FileHelper::onError);
    connect(f_socket, &QTcpSocket::readyRead, this, &FileHelper::onReadyRead);
    // 设置读写缓冲区大小
    int receiveBufferSize = 1024 * 1024; // 1MB
    int sendBufferSize = 1024 * 1024;   // 1MB
    f_socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, receiveBufferSize);
    f_socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, sendBufferSize);
    // 连接服务器
    f_socket->connectToHost(QHostAddress("127.0.0.1"), 8888);
    // 检查套接字状态
    if (!f_socket->waitForConnected(5000)) { // 等待5秒
        qDebug() << "Connection failed:" << f_socket->errorString();
        return;
    }
}
