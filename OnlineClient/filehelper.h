#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include "messagepackage.h"
#include <QFile>
#include <QTimer>
class FileHelper : public QObject
{
    Q_OBJECT
public:
    explicit FileHelper(bool send,bool group,QString filePath,QString fileName,QString sender,QString recver,QObject *parent = nullptr);
    ~FileHelper();
    void setFileSize(const int& fileSize);
    bool getSend(){
        return send;
    }
    bool getCancel(){
        return cancel;
    }
    QString getFileName(){
        return fileName;
    }
    void setSentSize(qint64 sentSize){
        this->sentSize=sentSize;
    }
    void setPause(bool pause){
        this->pause=pause;
    }
public slots:
    void onError(QTcpSocket::SocketError socketError) {
        QTcpSocket::SocketState state = f_socket->state();
        qDebug() << "Socket error: " << socketError;
        qDebug() << "Socket state: " << state;
    }
    void fileDeal();
    void onReadyRead();
    void onCancelFile();
    void onPauseFile();
signals:
    void fileSendFinished(MessagePackage& pack);
    void transferSpeedUpdated(double progressPercentage,double speedKBps,QString fileName,bool send); // 新增速率信号
private:
    void resultHandler(MessagePackage& pack);
    void fileSend();
    void fileRecv();
    void fileDownload(MessagePackage& pack);
    void fileUploadCancel();
    void updateTransferSpeed();
private:
    void initSocket();
    QTcpSocket* f_socket;
    bool send;
    bool pause=false;
    bool cancel;
    bool group;
    QString filePath;
    QString sender;
    QString fileName;
    QString recver;
    qint64 fileSize;
    qint64 sentSize;
    QString logFileName;

    QFile* file;
    QFile* log;

    // ...
    QElapsedTimer m_speedTimer;      // 计时器
    qint64 m_lastSentSize = 0;       // 上次统计时的已传输字节数
    double m_currentSpeed = 0.0;     // 当前速率（KB/s）
    QTimer* m_speedUpdateTimer;      // 速率更新定时器

    // 定义最大缓冲区大小
    static constexpr int MAX_BUFFER_SIZE = 1024 * 1024;  // 1 MB
};

#endif // FILEHELPER_H
