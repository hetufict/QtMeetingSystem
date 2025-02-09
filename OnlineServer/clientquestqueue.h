#ifndef CLIENTQUESTQUEUE_H
#define CLIENTQUESTQUEUE_H

#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include "clientsockethandler.h"

class ClientQuestHandler:public QObject
{
    Q_OBJECT
public:
    ClientQuestHandler(QObject* parent=nullptr);
    ~ClientQuestHandler()override;
    void addNewTask();
    bool taskIsEmpty();
    ClientSocketHandler* getTask();
    QMutex* getCQHlock_mutex();
    QWaitCondition* getCQHlock_cond();

private:
    QList<ClientSocketHandler*> CQHc_task_list;
    QMutex CQHlock_mutex();
    QWaitCondition CQHlock_cond;
};

class ClientQuestQueue : public QObject
{
    Q_OBJECT
public:
    explicit ClientQuestQueue(QObject *parent = nullptr);

signals:
};

#endif // CLIENTQUESTQUEUE_H
