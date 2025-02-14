#ifndef MESSAGEPACKAGE_H
#define MESSAGEPACKAGE_H
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QByteArray>
#include <QString>
#include <QTcpSocket>


class MessagePackage
{
public:
    MessagePackage(const char* data=nullptr,int size=0);
    MessagePackage(QByteArray& data);
    QJsonObject m_data;
    QString Type()const;//获取请求类型
    void setType(QString type);//设置请求类型
    //字符数据
    QString getStringValue(const QString& key)const;
    void addValue(QString key,QString value);
    //整形数据
    int getIntValue(QString key)const;
    void addValue(QString key,int value);
    //列表数据
    QStringList getListValue(const QString& key)const;
    void addValue(QString key,QStringList values);
    //文件数据
    //void addValue(const QString& key,const QString& filePath);
    void addValue(const QString &key, const QByteArray &fileData);
    QByteArray getFileValue(const QString& key) const;
    //发送包
    void sendMsg(QTcpSocket* socket);
    //接受包
    void recvMsg(QTcpSocket* socket);


    /***********键值类型变量**************/
    static QString Key_Type_Login;
    static QString Key_Type_Logout;
    static QString Key_Type_Register;
    static QString Key_Type_UpdateList;
    static QString Key_Type_getOnlineUsr;
    static QString Key_Type_PrivateChat;
    static QString Key_Type_AddGroup;
    static QString Key_Type_AddGroupMembers;
    static QString Key_Type_GetMyGroups;
    static QString Key_Type_GetGroupMembers;
    static QString Key_Type_InviteGroupMember;
    static QString Key_Type_GroupChat;
    static QString Key_Type_GroupFile;
    static QString Key_Type_PrivateFile;
    static QString Key_Type_GetFileList;
    static QString Key_Type_GetFile;
    static QString Key_Type_CreateMeeting;
    static QString Key_Type_InviteMeeting;
    static QString Key_Type_JoinMeeting;
    static QString Key_Type_CloseMeeting;
    static QString Key_Type_CleanMeeting;
    static QString Key_Type_MembersList;
    static QString Key_Type_FILEDATA;
    static QString Key_Type_FilePos;
    static QString Key_Type_FileOK;
    static QString Key_Type_FileDataRequest;

    static QString Key_Name;
    static QString Key_Pasd;
    static QString Key_Result;
    static QString Key_Action;
    static QString Key_UserList;
    static QString Key_Sender;
    static QString Key_Receiver;
    static QString Key_Message;
    static QString Key_GroupName;
    static QString Key_UserGroupList;
    static QString key_FileName;
    static QString key_FileData;
    static QString key_FileSize;
    static QString key_FileList;
    static QString key_SenderList;
    static QString key_MeetingName;
    static QString key_MeetingID;
    static QString Key_MessagePort;
    static QString Key_VideoPort;
    static QString Key_MediaPort;
    static QString Key_CloseMeeting;
    static QString Key_MeetingMembersIn;
    static QString Key_MeetingMembersAbsent;
    static QString Key_MeetingMembersLeave;
    static QString Key_SentSize;
};

#endif // MESSAGEPACKAGE_H
