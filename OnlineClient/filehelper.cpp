#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#include <QVariant>
#include "filehelper.h"
#include "messagepackage.h"
#include "logger.h"


FileHelper::FileHelper(bool send, bool group, QString filePath, QString fileName, QString sender, QString recver, QObject *parent)
    : QObject{parent}, send(send), filePath(filePath), fileName(fileName), sender(sender), recver(recver), group(group), sentSize(0)
{
    cancel=false;
    //打开文件
    if(send)
    {
        file=new QFile(filePath);
        if(!file->open(QIODevice::ReadOnly)){
            LOG(Logger::Error,"open file failed: "+filePath+file->errorString());
            return ;
        }
    }else{
        filePath=filePath+fileName;
        file=new QFile(filePath);
        if(!file->open(QIODevice::WriteOnly)){
            LOG(Logger::Error,"open file failed: "+filePath+file->errorString());
            return ;
        }
    }
    //打开日志文件 记录发送情况
    QString logFileName="file_process.txt";
    log=new QFile(logFileName);
    if(!log->open(QIODevice::WriteOnly)){
        qDebug()<<"无法打开日志文件"+log->errorString();
        file->close();
    }

    // 初始化速率计算
    m_speedTimer.start();
    m_speedUpdateTimer = new QTimer(this);
    connect(m_speedUpdateTimer, &QTimer::timeout, this, &FileHelper::updateTransferSpeed);
    m_speedUpdateTimer->start(1000); // 每秒更新一次速率
}

FileHelper::~FileHelper()
{
    m_speedUpdateTimer->stop();
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

void FileHelper::onCancelFile()
{
    cancel=true;
}

void FileHelper::onPauseFile()
{
    pause=!pause;
    LOG(Logger::Info,"set file process pause");
}

void FileHelper::resultHandler(MessagePackage &pack)
{
    if(pack.Type()==MessagePackage::Key_Type_FilePos){//收到接受确认
        fileSend();//发送下一个包
    }else if(pack.Type()==MessagePackage::Key_Type_FileOK){
        // 删除日志文件
        if (!log->remove()) {
            LOG(Logger::Error,"remove file failed: "+logFileName+log->errorString());
        }
        emit fileSendFinished(pack);
    }else if(pack.Type()==MessagePackage::Key_Type_FILEDATA){
        fileDownload(pack);
    }
}

void FileHelper::fileDeal()
{
    initSocket(); // 初始化 Socket
    if(send){
        fileSend();
    }else{
        fileRecv();
    }
}

void FileHelper::fileSend()
{
    while(pause||cancel){
        // 检查是否有待处理的事件
        QCoreApplication::processEvents();
        //如果取消发送，关闭文件，清理申请的资源
        if(cancel){
            fileUploadCancel();
            return;
        }
        //QThread::sleep(5);
    }
    //发送文件
    qint64 totalSize=file->size();
    const int bufferSize=8192;
    // 检查套接字状态
    if (f_socket->state() != QAbstractSocket::ConnectedState) {
        LOG(Logger::Error, QString("Socket error: %1 Socket error string: %2")
                .arg(f_socket->error()) // 将枚举值转换为字符串
                .arg(f_socket->errorString()));
        return;
    }
    // 定位到当前文件指针位置
    if (!file->seek(sentSize)) {
        LOG(Logger::Error,"file seek error: "+fileName+file->errorString());
        qDebug() << "无法定位到文件位置" << sentSize << "错误：" << file->errorString();
        return;
    }
    // 读取分块数据
    QByteArray buffer;
    int currentSize=std::min(bufferSize,(int)(totalSize-sentSize));
    buffer = file->read(currentSize);
    if (buffer.isEmpty()) {
        LOG(Logger::Error,"read file failed: empty buffer");
        qDebug() << "文件读取错误";
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
    LOG(Logger::Info, QString("buffer size: %1 sent size: %2 pack type: %3 socket descriptor: %4")
                          .arg(buffer.size())
                          .arg(sentSize)
                          .arg(pack.Type())
                          .arg(f_socket->socketDescriptor()));
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
    LOG(Logger::Info,"requset file pack");

}

void FileHelper::fileDownload(MessagePackage &pack)
{
    //设置文件存储路径
    QByteArray fileData=pack.getFileValue(MessagePackage::key_FileData);
    sentSize=pack.getIntValue(MessagePackage::Key_SentSize);
    // 写入数据
    file->seek(sentSize-fileData.size()); // 定位到文件的上次写入的位置
    file->write(fileData);   // 写入数据
    // 记录文件已接受大小，回复接受数据确认
    if(sentSize < fileSize){
        while(pause||cancel){
            // 检查是否有待处理的事件
            QCoreApplication::processEvents();
            //如果取消发送，关闭文件，清理申请的资源
            if(cancel){
                fileUploadCancel();
                return;
            }
        }
        fileRecv();//继续请求文件数据
    }
    else if (sentSize== fileSize){//如果文件上传完成
        // 删除日志文件
        if (!log->remove()) {
            LOG(Logger::Error,"remove file failed: "+logFileName+log->errorString());
        }
        emit fileSendFinished(pack);
    }
}

void FileHelper::fileUploadCancel()
{
    //通知服务器，处理相应资源
    MessagePackage pack;
    pack.setType(MessagePackage::Key_Type_CancelUpload);
    pack.addValue(MessagePackage::key_FileName,fileName);//文件名
    pack.addValue(MessagePackage::Key_Sender,sender);//发送者
    pack.addValue(MessagePackage::Key_Receiver,recver);//接受者
    pack.addValue(MessagePackage::Key_Result,group);//是否是群文件
    if(send){
        pack.sendMsg(f_socket);
    }
    emit fileSendFinished(pack);//通知主线程处理线程资源

    // 删除日志文件
    if (!log->remove()) {
        LOG(Logger::Error,"remove file failed: "+logFileName+log->errorString());
    }
    m_speedUpdateTimer->stop();
}

void FileHelper::updateTransferSpeed() {
    if (sentSize <= 0) return;

    qint64 elapsed = m_speedTimer.elapsed();
    if (elapsed == 0) return;

    // 计算瞬时速率（KB/s）
    double speed = (sentSize - m_lastSentSize) / 1024.0 / (elapsed / 1000.0);
    m_lastSentSize = sentSize;
    m_speedTimer.restart();

    // 平滑处理（可选）
    m_currentSpeed = 0.8 * m_currentSpeed + 0.2 * speed;

    // 计算进度百分比
    double progressPercentage = (static_cast<double>(sentSize) / fileSize) * 100.0;

    // 准备日志信息
    QString logText = QString("send=%1;group=%2;filePath=%3;fileName=%4;sender=%5;receiver=%6;sentSize=%7;fileSize=%8")
                          .arg(send)
                          .arg(group)
                          .arg(filePath)
                          .arg(fileName)
                          .arg(sender)
                          .arg(recver)
                          .arg(sentSize)
                          .arg(fileSize);

    // 写入日志文件
    if (log->isOpen()) {
        log->seek(0); // 定位到文件开头
        log->write(logText.toUtf8()); // 写入日志信息
        log->flush(); // 立即写入磁盘
    } else {
        qDebug() << "日志文件未打开，无法写入日志";
    }

    // 发送速率更新信号
    emit transferSpeedUpdated(progressPercentage, m_currentSpeed, fileName, send);
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
        LOG(Logger::Error,"Connection failed :"+f_socket->errorString());
        return;
    }
}
