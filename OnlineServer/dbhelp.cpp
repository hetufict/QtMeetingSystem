#include "dbhelper.h"
#include <QDebug>

dbhelper* dbhelper::instance = nullptr;

dbhelper *dbhelper::getInstance()
{
    if(instance == nullptr)
    {
        instance = new dbhelper;
    }
    return instance;
}

bool dbhelper::userLogin(QString name,QString pswd)
{
    QSqlQuery query(db);
    QString str = QString("select * from %1 where name='%2' and pswd='%3';").arg(userTableName).arg(name).arg(pswd);
    qDebug()<<"用户登录检测"<<query.exec(str);
    return query.next();
}

bool dbhelper::addFriend(QString name1, QString name2)
{
    QSqlQuery query(db);
    QString str = QString("select * from %1 where name1='%2' and name2='%3';").arg(friendTableName).arg(name1).arg(name2);

    query.exec(str);
    if(query.next())
    {
        qDebug()<<"好友关系已存在";
        return false;
    }

    str = QString("insert into %1 values('%2','%3');").arg(friendTableName).arg(name1).arg(name2);
    bool ret = query.exec(str);
    str = QString("insert into %1 values('%2','%3');").arg(friendTableName).arg(name2).arg(name1);
    ret = query.exec(str);

    qDebug()<<"添加好友"<<ret;
    return ret;
}

QStringList dbhelper::getFriends(QString name)
{
    QSqlQuery query(db);
    QString str = QString("select name2 from %1 where name1='%2';").arg(friendTableName).arg(name);

    qDebug()<<"获取好友列表"<<query.exec(str);
    QStringList list;
    while(query.next())
    {
        list.append(query.value(0).toString());
    }
    return list;
}

QStringList dbhelper::getUsers()
{
    QSqlQuery query(db);
    QString str = QString("select name from %1;").arg(userTableName);
    qDebug()<<"获取所有用户"<<query.exec(str);
    QStringList list;
    while(query.next())
    {
        list.append(query.value(0).toString());
    }
    return list;
}

dbhelper::dbhelper()
{
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setUserName("root");
    db.setPassword("root");
    db.setDatabaseName("xw2402");

    if(!db.open())
    {
        qDebug()<<"数据库打开失败"<<db.lastError();
        return ;
    }

    //表格初始化
    userTableInit();
    friendTableInit();
}

dbhelper::~dbhelper()
{
    if(db.isOpen())
    {
        db.close();
    }
}

void dbhelper::userTableInit()
{
    QSqlQuery query(db);
    QString str = "create table if not exists %1(name varchar(64) primary key,pswd varchar(64) not null,online bool);";
    str = QString(str).arg(userTableName);
    qDebug()<<"初始化用户数据表"<<query.exec(str);
}

void dbhelper::friendTableInit()
{
    QSqlQuery query(db);
    QString str = "create table if not exists %1(name1 varchar(64) not null,name2 varchar(64) not null);";
    str = QString(str).arg(friendTableName);
    qDebug()<<"初始化好友关系数据表"<<query.exec(str);
}
