#ifndef DBHELPER_H
#define DBHELPER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QString>
#include <QMutex>
#include <QMutexLocker>
#include <QVector>
class DBHelper
{
public:
    static DBHelper *getInstance();
    int userLogin(const QString& name, const QString& pswd);//用户登陆处理
    bool userRegister(QString name, QString pswd);//用户注册
    bool userIsOnline(const QString& name);//查看用户是否在线
    bool userOffline(const QString& name);//设置用户离线状态
    QStringList getUsers();

    //群组操作
    bool addGroup(const QString& groupname);//添加新群组
    void addGroupMember(const QString &group, const QString &name);//添加群成员
    QStringList getGroupMembers(const QString &group);//获取群成员
    QStringList getGroupNames(const QString &user);//获取用户加入的群组

    //文件
    void addFile(const QString& fliePath,const QString& flieName,const QString& flieType,
                 const QString& flieSender,const QString& flieReceiver,const int& flieSize);
    QStringList getFileList(const QString &user1, const QString &user2,bool group);
    QStringList getSenderList(const QStringList &fileList, const QString &user1, const QString &user2,bool group);

    //会议
    bool ports[3000]={};
    int addMeeting(const QString &meet, const QString &host,const int& port1,const int& port2,const int& port3);
    bool inviteMeeting(const int& meetingID,const QString &meetingName,const QString &member,bool joined=false);
    bool isMemberExists(const int& meetingID,const QString &meetingName,const QString &member);
    bool isMemberJoined(const int& meetingID,const QString &meetingName,const QString &member);
    bool joinMeeting(const int& meetingID,const QString &meetingName,const QString &member);
    int* getMeetingPorts(const int& meetingID,const QString &meetingName);
    bool closeMeeting(const int& meetingID,const QString &meetingName);
    int leaveMeeting(const int& meetingID,const QString &meetingName,const QString &member);
    QStringList getMeetingMembers(const int& meetingID,const QString &meetingName);
    void clearMeetingRoom(const int& meetingID,const QString &meetingName,const QStringList &members);
    QStringList getInMembers(const int& meetingID,const QString &meetingName);
    QStringList getAbsentMembers(const int& meetingID,const QString &meetingName);
    QStringList getLeaveMembers(const int& meetingID,const QString &meetingName);
private:
    static DBHelper* instance;
    static QMutex mutex; // 用于同步的互斥锁
    DBHelper();
    ~DBHelper();

    QSqlDatabase db;
    void userTableInit();
    void groupTableInit();
    void fileTableInit();
    void meetingTableInit();

    QString userTableName = "userinfo";


};
#endif // DBHELPER_H
