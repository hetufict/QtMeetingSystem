#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include "messagepackage.h"
#include <QFile>
class FileHelper : public QObject
{
    Q_OBJECT
public:
    explicit FileHelper(bool send,bool group,QString filePath,QString fileName,QString sender,QString recver,QObject *parent = nullptr);
public slots:
    void onError(QTcpSocket::SocketError socketError) {
        QTcpSocket::SocketState state = f_socket->state();
        qDebug() << "Socket error: " << socketError;
        qDebug() << "Socket state: " << state;
    }
    void fileDeal();
    void onBytesWritten(qint64 bytes);
private:
    void fileSend();
    //void sendNextChunk();
    void fileRecv();
private:
    QTcpSocket* f_socket;
    bool send;
    bool pause=false;
    bool group;
    QString filePath;
    QString sender;
    QString fileName;
    QString recver;
    QFile file;
    qint64 fileSize;
    qint64 sentSize;
    QString logFileName;
    QTextStream logStream;

    // 定义最大缓冲区大小
    static constexpr int MAX_BUFFER_SIZE = 1024 * 1024;  // 1 MB
signals:
};

#endif // FILEHELPER_H
