#ifndef CLIENTTASK_H
#define CLIENTTASK_H

#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QTcpSocket>
#include "messagepackage.h"
#include "dbhelper.h"
class ClientTask : public QObject,public QRunnable
{
    Q_OBJECT
public:
    explicit ClientTask(QObject *parent = nullptr,QTcpSocket* socket=nullptr);
    virtual void run()override;
signals:

private:
    void handler(MessagePackage pack);
    QTcpSocket* socket;
    MessagePackage pack;
    void loginHandler(MessagePackage pack);
};

#endif // CLIENTTASK_H
