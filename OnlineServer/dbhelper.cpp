#include "dbhelper.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
DBHelper* DBHelper::instance = nullptr;
QMutex DBHelper::mutex;
DBHelper *DBHelper::getInstance()
{
    if (instance == nullptr) {
        QMutexLocker locker(&mutex); // 自动锁定互斥锁
        if (instance == nullptr) { // 再次检查 instance 是否为 nullptr
            instance = new DBHelper();
        }
    }
    return instance;
}

int DBHelper::userLogin(const QString& name, const QString& pswd)
{
    QSqlQuery query(db);

    // 首先检查用户是否存在
    query.prepare(QString("SELECT * FROM %1 WHERE name = :name AND pswd = :pswd").arg(userTableName));
    query.bindValue(":name", name);
    query.bindValue(":pswd", pswd);

    if (!query.exec()) {
        qWarning() << "查询执行失败:" << query.lastError();
        return -1; // 返回-1表示查询执行失败
    }

    if (query.next()) {
        // 用户存在，检查用户是否已经在线
        if (query.value("online").toBool()) {
            return 2; // 返回2表示用户已在线
        }
        // 用户不在线，更新在线状态为true
        query.prepare(QString("UPDATE %1 SET online = true WHERE name = :name").arg(userTableName));
        query.bindValue(":name", name);
        if (!query.exec()) {
            qWarning() << "更新在线状态失败:" << query.lastError();
            return -1; // 返回-1表示更新失败
        }
        return 1; // 返回1表示登录成功
    } else {
        return 0; // 返回0表示用户不存在
    }
}

bool DBHelper::userRegister(QString name, QString pswd)
{
    // 首先检查用户名是否已存在
    QSqlQuery query(db);
    query.prepare("SELECT * FROM " + userTableName + " WHERE name = :name");
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << "查询失败:" << query.lastError();
        return false;
    }

    if (query.next()) {
        // 用户名已存在
        return false;
    }

    // 用户名不存在，插入新用户
    query.prepare("INSERT INTO " + userTableName + " (name, pswd, online) VALUES (:name, :pswd, :online)");
    query.bindValue(":name", name);
    query.bindValue(":pswd", pswd);
    query.bindValue(":online", false); // 假设初始在线状态为false
    if (!query.exec()) {
        qWarning() << "注册失败:" << query.lastError();
        return false;
    }
    return true;
}

bool DBHelper::userIsOnline(const QString &name)
{
    QSqlQuery query(db);
    // 准备查询语句，检查用户在线状态
    query.prepare(QString("SELECT online FROM %1 WHERE name = :name").arg(userTableName));
    query.bindValue(":name", name);

    if (!query.exec()) {
        qWarning() << "查询执行失败:" << query.lastError();
        return false; // 查询失败，返回 false 表示无法确定在线状态
    }

    if (query.next()) {
        // 获取查询结果中的在线状态
        bool isOnline = query.value(0).toBool();
        return isOnline;
    } else {
        // 用户不存在
        return false;
    }
}

bool DBHelper::userOffline(const QString &name)
{
    QSqlQuery query(db);
    // 用户不在线，更新在线状态为true
    query.prepare(QString("UPDATE %1 SET online = false WHERE name = :name").arg(userTableName));
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << "更新在线状态失败:" << query.lastError();
        return false; // 返回-1表示更新失败
    }
    return true;
}

QStringList DBHelper::getUsers()
{
    QSqlQuery query(db);
    // 使用参数化查询（尽管在这个例子中不是必需的）
    QString str = QString("SELECT name FROM %1;").arg(userTableName);
    qDebug() << "执行查询：" << str;

    if (!query.exec(str)) {
        qWarning() << "查询失败:" << query.lastError();
        return QStringList(); // 查询失败时返回空列表
    }

    QStringList list;
    while (query.next()) {
        list.append(query.value(0).toString());
    }
    return list;
}

QStringList DBHelper::getOnlineUsers()
{
    QSqlQuery query(db);
    // 使用参数化查询（尽管在这个例子中不是必需的）
    QString str = QString("SELECT name FROM %1 where online=1;").arg(userTableName);
    qDebug() << "执行查询：" << str;

    if (!query.exec(str)) {
        qWarning() << "查询失败:" << query.lastError();
        return QStringList(); // 查询失败时返回空列表
    }

    QStringList list;
    while (query.next()) {
        list.append(query.value(0).toString());
    }
    return list;
}

QStringList DBHelper::getOfflineUsers()
{
    QSqlQuery query(db);
    // 使用参数化查询（尽管在这个例子中不是必需的）
    QString str = QString("SELECT name FROM %1 where online=0;").arg(userTableName);
    qDebug() << "执行查询：" << str;

    if (!query.exec(str)) {
        qWarning() << "查询失败:" << query.lastError();
        return QStringList(); // 查询失败时返回空列表
    }

    QStringList list;
    while (query.next()) {
        list.append(query.value(0).toString());
    }
    return list;
}

bool DBHelper::addGroup(const QString& groupname)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO groupInfo (name, num) VALUES (:groupname, 0)");
    query.bindValue(":groupname", groupname);
    if (!query.exec()) {
        qWarning() << "添加群组失败:" << query.lastError();
        return false;
    }
    return true;
}

void DBHelper::addGroupMember(const QString &group, const QString &name) {
    QSqlQuery query(db);

    // 首先检查群组是否存在
    query.prepare("SELECT num FROM groupInfo WHERE name = :group");
    query.bindValue(":group", group);
    if (!query.exec()) {
        qWarning() << "查询失败:" << query.lastError();
        return;
    }

    if (query.next()) {
        int currentNum = query.value(0).toInt();
        // 更新群组人数
        query.prepare("UPDATE groupInfo SET num = :newNum WHERE name = :group");
        query.bindValue(":newNum", currentNum + 1);
        query.bindValue(":group", group);
        if (!query.exec()) {
            qWarning() << "更新群组人数失败:" << query.lastError();
            return;
        }
    } else {
        qWarning() << "群组不存在";
        return;
    }

    // 添加新成员到群成员表
    query.prepare("INSERT INTO groupMembers (groupname, username) VALUES (:group, :name)");
    query.bindValue(":group", group);
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << "添加群成员失败:" << query.lastError();
    }
}

QStringList DBHelper::getGroupMembers(const QString &group)
{
    QStringList members;
    QSqlQuery query(db);
    query.prepare("SELECT username FROM groupMembers WHERE groupname = :group");
    query.bindValue(":group", group);
    if (query.exec()) {
        while (query.next()) {
            members.append(query.value(0).toString());
        }
    }
    return members;
}

QStringList DBHelper::getGroupNames(const QString &user)
{
    QStringList groups;
    QSqlQuery query(db);
    query.prepare("SELECT groupname FROM groupMembers WHERE username = :user");
    query.bindValue(":user", user);
    if (query.exec()) {
        while (query.next()) {
            groups.append(query.value(0).toString());
        }
    }
    return groups;
}

void DBHelper::addFile(const QString& filePath, const QString& fileName,
                       const QString& fileType, const QString& fileSender,
                       const QString& fileReceiver, const int& fileSize) {
    QSqlQuery query(db);

    // 准备 SQL 插入语句
    query.prepare("INSERT INTO fileInfo (filepath, filename, sender, receiver, filetype, filesize) "
                  "VALUES (:filepath, :filename, :sender, :receiver, :filetype, :filesize)");

    // 绑定参数
    query.bindValue(":filepath", filePath);
    query.bindValue(":filename", fileName);
    query.bindValue(":sender", fileSender);
    query.bindValue(":receiver", fileReceiver);
    query.bindValue(":filetype", fileType);
    query.bindValue(":filesize", fileSize);

    // 执行插入操作
    if (!query.exec()) {
        qWarning() << "添加文件信息失败:" << query.lastError();
    }
}

QStringList DBHelper::getFileList(const QString &user1, const QString &user2, bool group) {
    QStringList fileList;
    QSqlQuery query(db);
    if(group){
        // 基础查询条件，用于找到双向关系中的文件
        QString baseQuery = "SELECT filename FROM fileInfo WHERE receiver = :user2 AND filetype = 'group';";
        query.prepare(baseQuery);
        query.bindValue(":user2", user2);

        if (query.exec()) {
            while (query.next()) {
                fileList.append(query.value(0).toString());
            }
        } else {
            qDebug() << "查询失败:" << query.lastError();
        }
    }else{
        // 私聊文件查询优化
        QString baseQuery =
            "SELECT filename FROM fileInfo "
            "WHERE filetype = 'private' "
            "AND ("
            "    (sender = :user1 AND receiver = :user2) OR "
            "    (sender = :user2 AND receiver = :user1)"
            ")";
        query.prepare(baseQuery);
        query.bindValue(":user1", user1);
        query.bindValue(":user2", user2);

        if (query.exec()) {
            while (query.next()) {
                fileList.append(query.value(0).toString());
            }
        } else {
            qDebug() << "查询失败:" << query.lastError();
        }

    }
    qDebug() << "File List Size:" << fileList.size();
    return fileList;
}

QStringList DBHelper::getFileList(const QString &groupName)
{
    return getFileList("",groupName,true);
}

QStringList DBHelper::getSenderList(const QStringList &fileList, const QString &user1, const QString &user2, bool group) {
    QStringList senderList;
    QSqlQuery query(db);

    if(group){
        QString baseQuery = "SELECT sender FROM fileInfo WHERE receiver = :user2 AND filetype = 'group';";
        query.prepare(baseQuery);
        query.bindValue(":user2", user2);

        if (query.exec()) {
            while (query.next()) {
                senderList.append(query.value(0).toString());
            }
        } else {
            qDebug() << "查询失败:" << query.lastError();
        }

    }else{
        // 检查文件列表是否为空
        if (fileList.isEmpty()) {
            qDebug() << "文件列表为空";
            return senderList;
        }
        // 构建一个包含所有文件的 OR 条件字符串
        QString fileCondition;
        for (int i = 0; i < fileList.size(); ++i) {
            if (!fileCondition.isEmpty()) {
                fileCondition += " OR ";
            }
            QString temp = fileList[i];
            fileCondition += QString("filename = '%1'").arg(temp.replace("'", "''")); // 转义单引号
        }
        // 构建 SQL 查询字符串
        QString str = "SELECT sender FROM fileInfo "
                      "WHERE (" + fileCondition + ") "
                                        "AND ("
                                        "(sender = :user1 AND receiver  = :user2) OR"
                                        "(sender = :user2 AND receiver = :user1)"
                                        ")"
                                        "AND filetype = 'private'";
        query.prepare(str);
        query.bindValue(":user1", user1);
        query.bindValue(":user2", user2);
        qDebug() << "Executing SQL:" << str;
        if (query.exec()) {
            while (query.next()) {
                senderList.append(query.value(0).toString());
            }
        } else {
            qDebug() << "查询失败:" << query.lastError();
        }
        qDebug() << "Sender List Size:" << senderList.size();

    }
    return senderList;
}

int DBHelper::addMeeting(const QString &meet, const QString &host,const int& port1,const int& port2,const int& port3)
{
    QSqlQuery query(db);
    // 准备 SQL 插入语句
    query.prepare("INSERT INTO meetingInfo (meetingname, hostname, messageport, videoport,mediaport) "
                  "VALUES (:meetingname, :hostname, :messageport, :videoport, :mediaport)");

    // 绑定参数
    query.bindValue(":meetingname", meet);
    query.bindValue(":hostname", host);
    query.bindValue(":messageport", QString::number(port1));
    query.bindValue(":videoport", QString::number(port2));
    query.bindValue(":mediaport", QString::number(port3));

    // 执行插入操作
    bool ret=query.exec();
    if (!ret) {
        qWarning() << "添加文件信息失败:" << query.lastError();
        return -1;
    }
    // 获取自增ID
    QSqlQuery lastQuery(db);
    lastQuery.prepare("SELECT LAST_INSERT_ID() AS lastId");
    if (lastQuery.exec() && lastQuery.next()) {
        int meetingId = lastQuery.value(0).toInt();
        return meetingId; // 返回会议ID
    } else {
        qWarning() << "获取会议ID失败:" << lastQuery.lastError();
    }
    return -1; // 返回-1表示获取ID失败
}

bool DBHelper::inviteMeeting(const int &meetingID, const QString &meetingName, const QString &member,bool joined) {

    QSqlQuery query(db);

    // 准备 SQL 插入语句
    query.prepare("INSERT INTO meetingmembers (meetingid, meetingname, membername,inmeeting) "
                  "VALUES (:meetingid, :meetingname, :membername, :inmeeting)");

    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);
    query.bindValue(":membername", member);
    query.bindValue(":inmeeting", joined);

    // 执行插入操作
    bool ret = query.exec();
    if (!ret) {
        qWarning() << "邀请会议失败:" << query.lastError();
    }
    return ret;
}

bool DBHelper::isMemberExists(const int &meetingID, const QString &meetingName, const QString &member) {
    QSqlQuery query(db);

    // 准备 SQL 查询语句，检查 meetingid, meetingname, membername，并且确保 inmeeting 不为 0
    query.prepare("SELECT COUNT(*) FROM meetingmembers WHERE meetingid = :meetingid "
                  "AND meetingname = :meetingname AND membername = :membername AND inmeeting <> 0");

    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);
    query.bindValue(":membername", member);

    // 执行查询操作
    if (!query.exec()) {
        qWarning() << "检查成员是否存在失败:" << query.lastError();
        return false; // 如果查询执行失败，直接返回 false
    }

    // 检查查询结果
    if (query.next()) {
        int count = query.value(0).toInt();
        return count > 0; // 如果计数大于0，表示成员存在且 inmeeting 不为 0
    }
    // 如果查询没有返回结果，说明成员不存在或者 inmeeting 为 0
    return false;
}

bool DBHelper::isMemberJoined(const int &meetingID, const QString &meetingName, const QString &member) {
    QSqlQuery query(db);

    // 准备 SQL 查询语句
    query.prepare("SELECT inmeeting FROM meetingmembers WHERE meetingid = :meetingid "
                  "AND meetingname = :meetingname AND membername = :membername");

    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);
    query.bindValue(":membername", member);

    // 执行查询操作
    if (!query.exec()) {
        qWarning() << "检查成员是否已加入会议失败:" << query.lastError();
        return false;
    }

    // 检查查询结果
    if (query.next()) {
        int inmeeting = query.value(0).toInt();
        return inmeeting != 1; // 如果inmeeting不为1，返回true
    }

    return false; // 如果没有查询结果，表示成员不存在
}

bool DBHelper::joinMeeting(const int &meetingID, const QString &meetingName, const QString &member) {
    QSqlQuery query(db);

    // 准备 SQL 更新语句
    query.prepare("UPDATE meetingmembers SET inmeeting = 1 WHERE meetingid = :meetingid "
                  "AND meetingname = :meetingname AND membername = :membername");

    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);
    query.bindValue(":membername", member);

    // 执行更新操作
    bool ret = query.exec();
    if (!ret) {
        qWarning() << "加入会议失败:" << query.lastError();
    }
    return ret;
}

int *DBHelper::getMeetingPorts(const int &meetingID, const QString &meetingName) {
    QSqlQuery query(db);

    // 准备 SQL 查询语句
    query.prepare("SELECT messageport, videoport, mediaport FROM meetingInfo WHERE id = :meetingid AND meetingname = :meetingname");

    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);
    // 执行查询操作
    if (!query.exec()) {
        qWarning() << "获取会议端口失败:" << query.lastError();
        return nullptr;
    }
    // 检查查询结果
    if (query.next()) {
        int messagePort = query.value(0).toInt();
        int videoPort = query.value(1).toInt();
        int mediaPort = query.value(2).toInt();

        // 分配内存以存储端口信息
        int *ports = new int[3];
        ports[0] = messagePort;
        ports[1] = videoPort;
        ports[2] = mediaPort;
        qDebug()<<"sql:port1"<<ports[0]<<"sql:port2"<<ports[1]<<"sql:port3"<<ports[2];
        return ports; // 返回包含端口信息的数组
    }
   qDebug()<<"sql:port1 null";
    return nullptr; // 如果没有查询结果，返回nullptr
}

bool DBHelper::closeMeeting(const int &meetingID, const QString &meetingName) {
    QSqlQuery query(db);

    // 获取端口信息
    query.prepare("SELECT messageport, videoport, mediaport FROM meetingInfo WHERE id = :meetingid AND meetingname = :meetingname");
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);
    if (!query.exec()) {
        qWarning() << "获取会议端口失败:" << query.lastError();
        return false;
    }

    if (query.next()) {
        int messagePort = query.value(0).toInt();
        int videoPort = query.value(1).toInt();
        int mediaPort = query.value(2).toInt();

        // 更新端口状态
        ports[messagePort-1000]=false;
        ports[videoPort-1000]=false;
        ports[mediaPort-1000]=false;
        // 更新会议的start状态
        query.prepare("UPDATE meetingInfo SET start = 0 WHERE id = :meetingid AND meetingname = :meetingname");
        query.bindValue(":meetingid", meetingID);
        query.bindValue(":meetingname", meetingName);
        if (!query.exec()) {
            qWarning() << "关闭会议失败:" << query.lastError();
            return false;
        }

        if (query.numRowsAffected() > 0) {
            return true;
        } else {
            qWarning() << "没有找到匹配的会议记录";
            return false;
        }
    } else {
        qWarning() << "没有找到匹配的会议记录";
        return false;
    }
}

int DBHelper::leaveMeeting(const int &meetingID, const QString &meetingName, const QString &member) {
    QSqlQuery query(db);
    query.prepare("UPDATE meetingmembers SET inmeeting = 2 WHERE meetingid = :meetingid "
                  "AND meetingname = :meetingname AND membername = :membername");
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);
    query.bindValue(":membername", member);
    //qDebug()<<"离开会议++++"<<meetingID<<meetingName<<member;
    if (!query.exec()) {
        qWarning() << "离开会议失败:" << query.lastError();
        return -1; // 返回错误代码
    }

    if (query.numRowsAffected() > 0) {
        return 2; // 返回成功代码
    } else {
        return 0; // 没有记录被更新
    }
}

QStringList DBHelper::getMeetingMembers(const int &meetingID, const QString &meetingName) {
    QSqlQuery query(db);
    QStringList members;
    // 准备 SQL 查询语句
    query.prepare("SELECT membername FROM meetingmembers WHERE meetingid = :meetingid AND meetingname = :meetingname AND inmeeting = 1");
    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);

    // 执行查询操作
    if (!query.exec()) {
        qWarning() << "获取会议成员失败:" << query.lastError();
        return members; // 返回空列表
    }
    // 检查查询结果
    while (query.next()) {
        members.append(query.value(0).toString());
    }

    return members; // 返回成员列表
}

void DBHelper::clearMeetingRoom(const int &meetingID, const QString &meetingName, const QStringList &members) {
    QSqlQuery query(db);

    // 对每个成员执行更新操作
    foreach (const QString &member, members) {
        query.prepare("UPDATE meetingmembers SET inmeeting = 2 WHERE meetingid = :meetingid AND meetingname = :meetingname AND membername = :membername");
        query.bindValue(":meetingid", meetingID);
        query.bindValue(":meetingname", meetingName);
        query.bindValue(":membername", member);

        if (!query.exec()) {
            qWarning() << "更新成员状态失败:" << query.lastError();
        }
    }
}

QStringList DBHelper::getInMembers(const int &meetingID, const QString &meetingName) {
    QSqlQuery query(db);
    QStringList members;

    // 准备 SQL 查询语句
    query.prepare("SELECT membername FROM meetingmembers WHERE meetingid = :meetingid "
                  "AND meetingname = :meetingname AND inmeeting = 1");

    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);

    // 执行查询操作
    if (!query.exec()) {
        qWarning() << "获取会议成员失败:" << query.lastError();
        return members; // 返回空列表
    }

    // 检查查询结果
    while (query.next()) {
        QString memberName = query.value(0).toString();
        members.append(memberName);
    }

    return members; // 返回成员列表
}

QStringList DBHelper::getAbsentMembers(const int &meetingID, const QString &meetingName)
{
    QSqlQuery query(db);
    QStringList members;

    // 准备 SQL 查询语句
    query.prepare("SELECT membername FROM meetingmembers WHERE meetingid = :meetingid "
                  "AND meetingname = :meetingname AND inmeeting = 0");

    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);

    // 执行查询操作
    if (!query.exec()) {
        qWarning() << "获取会议成员失败:" << query.lastError();
        return members; // 返回空列表
    }

    // 检查查询结果
    while (query.next()) {
        QString memberName = query.value(0).toString();
        members.append(memberName);
    }

    return members; // 返回成员列表
}

QStringList DBHelper::getLeaveMembers(const int &meetingID, const QString &meetingName)
{
    QSqlQuery query(db);
    QStringList members;

    // 准备 SQL 查询语句
    query.prepare("SELECT membername FROM meetingmembers WHERE meetingid = :meetingid "
                  "AND meetingname = :meetingname AND inmeeting = 2");

    // 绑定参数
    query.bindValue(":meetingid", meetingID);
    query.bindValue(":meetingname", meetingName);

    // 执行查询操作
    if (!query.exec()) {
        qWarning() << "获取会议成员失败:" << query.lastError();
        return members; // 返回空列表
    }

    // 检查查询结果
    while (query.next()) {
        QString memberName = query.value(0).toString();
        members.append(memberName);
    }

    return members; // 返回成员列表
}

DBHelper::DBHelper()
{
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setUserName("root");
    db.setPassword("123456");
    db.setPort(3306);
    db.setDatabaseName("onlineserver");

    if(!db.open())
    {
        qDebug()<<"数据库打开失败"<<db.lastError();
        return ;
    }

    //表格初始化
    userTableInit();
    groupTableInit();
    fileTableInit();
    meetingTableInit();
}

DBHelper::~DBHelper()
{
    if(db.isOpen())
    {
        db.close();
    }
}

void DBHelper::userTableInit()
{
    QSqlQuery query(db);
    QString str = "create table if not exists %1(name varchar(64) primary key,pswd varchar(64) not null,online bool);";
    str = QString(str).arg(userTableName);
    if(!query.exec(str))
    {
        qDebug()<<"table initialize"<<query.lastError();
    }
}

void DBHelper::groupTableInit()
{
    QSqlQuery query(db);

    // 群基本信息
    QString str = "create table if not exists groupInfo("
                  "name varchar(64) primary key, "
                  "num int default 0);";
    if(!query.exec(str))
    {
        qDebug()<<"table initialize"<<query.lastError();
    }

    // 群成员信息
    str = "create table if not exists groupMembers("
          "groupname varchar(64) not null, "
          "username varchar(64) not null, "
          "primary key (groupname, username), "
          "foreign key (groupname) references groupInfo(name) on delete cascade);";
    if(!query.exec(str))
    {
        qDebug()<<"table initialize"<<query.lastError();
    }
}

void DBHelper::fileTableInit()
{
    QSqlQuery query(db);
    QString str = "create table if not exists fileInfo("
                  "filepath varchar(255) primary key, filename varchar(64) not null,"
                  "sender varchar(64) not null,receiver varchar(64) not null,"
                  "filetype varchar(64) not null,filesize int default 0);";
    if(!query.exec(str))
    {
        qDebug()<<"table initialize"<<query.lastError();
    }
}

void DBHelper::meetingTableInit()
{
    QSqlQuery query(db);

    // 群基本信息
    QString str = "create table if not exists meetingInfo("
                  "id int auto_increment primary key,"
                  "meetingname varchar(64) not null, "
                  "hostname varchar(64) not null,"
                  "messageport int not null,"
                  "videoport int not null,"
                  "mediaport int not null,"
                  "start int default 1);";
    if(!query.exec(str))
    {
        qDebug()<<"table initialize"<<query.lastError();
    }
    // 群成员信息
    str = "create table if not exists meetingmembers("
          "times int auto_increment primary key,"
          "meetingid int not null,"
          "meetingname varchar(64) not null, "
          "membername varchar(64) not null, "
          "inmeeting int default 0);";
    if(!query.exec(str))
    {
        qDebug()<<"table initialize"<<query.lastError();
    }
}

