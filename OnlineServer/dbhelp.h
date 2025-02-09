#ifndef DBHELP_H
#define DBHELP_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

class dbhelper
{
public:
    static dbhelper *getInstance();
    bool userLogin(QString name, QString pswd);
    bool addFriend(QString name1,QString name2);
    QStringList getFriends(QString name);
    QStringList getUsers();

private:
    static dbhelper* instance;
    dbhelper();
    ~dbhelper();

    QSqlDatabase db;
    void userTableInit();
    void friendTableInit();

    QString userTableName = "userinfo";
    QString friendTableName = "friendlist";

};

#endif // DBHELP_H
