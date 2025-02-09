#include "messagepackage.h"
#include <QDebug>
QString MessagePackage::Key_Type_Login="user login";
QString MessagePackage::Key_Type_Logout="user logout";
QString MessagePackage::Key_Type_Register="user register";
QString MessagePackage::Key_Type_UpdateList="update_list";
QString MessagePackage::Key_Type_getOnlineUsr="get_user_list";
QString MessagePackage::Key_Type_PrivateChat="user private chat";
QString MessagePackage::Key_Type_AddGroup="add a Group";
QString MessagePackage::Key_Type_AddGroupMembers="add a group member";
QString MessagePackage::Key_Type_GetMyGroups="get my groups";
QString MessagePackage::Key_Type_GetGroupMembers="get groupMembers";
QString MessagePackage::Key_Type_InviteGroupMember="invite group member";
QString MessagePackage::Key_Type_GroupChat="user group chat";
QString MessagePackage::Key_Type_GroupFile="user group flie";
QString MessagePackage::Key_Type_PrivateFile="user private flie";
QString MessagePackage::Key_Type_GetFileList="get file list";
QString MessagePackage::Key_Type_GetFile="get file";
QString MessagePackage::Key_Type_CreateMeeting="create_a_meeting";
QString MessagePackage::Key_Type_InviteMeeting="invite to meeting";
QString MessagePackage::Key_Type_JoinMeeting="join meeting";
QString MessagePackage::Key_Type_CloseMeeting="close meeting";
QString MessagePackage::Key_Type_CleanMeeting="clean meeting room";
QString MessagePackage::Key_Type_MembersList="update_members_list";
QString MessagePackage::Key_Type_FILEDATA="file data";


QString MessagePackage::Key_Name="name";
QString MessagePackage::Key_Pasd="pswd";
QString MessagePackage::Key_Result="result";
QString MessagePackage::Key_Action="action";
QString MessagePackage::Key_UserList="userlist";
QString MessagePackage::Key_Sender="sender";
QString MessagePackage::Key_Receiver="rceiver";
QString MessagePackage::Key_Message="message";
QString MessagePackage::Key_GroupName="groupname";
QString MessagePackage::Key_UserGroupList="group list";
QString MessagePackage:: key_FileName="filename";
QString MessagePackage:: key_FileData="filedata";
QString MessagePackage:: key_FileSize="filesize";
QString MessagePackage::key_FileList="flielist";
QString MessagePackage::key_SenderList="senderlist";
QString MessagePackage::key_MeetingName="meetingname";
QString MessagePackage::key_MeetingID="meetingid";
QString MessagePackage::Key_MessagePort="messageport";
QString MessagePackage::Key_VideoPort="videoport";
QString MessagePackage::Key_MediaPort="mediaport";
QString MessagePackage::Key_CloseMeeting="closemeet";
QString MessagePackage::Key_MeetingMembersIn="membersin";
QString MessagePackage::Key_MeetingMembersAbsent="membersabsent";
QString MessagePackage::Key_MeetingMembersLeave="membersleave";
QString MessagePackage::Key_SentSize="filesentsize";
MessagePackage::MessagePackage(const char *data, int size)
{
    if (data != nullptr && size > 0)
    {
        QByteArray jsonData(data, size);
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (!doc.isNull()) // 确保解析成功
        {
            m_data = doc.object(); // 从 QJsonDocument 获取 QJsonObject
        }
        else
        {
            // 处理 JSON 解析错误
        }
    }
}

MessagePackage::MessagePackage(QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isNull()) // 确保解析成功
    {
        m_data = doc.object(); // 从 QJsonDocument 获取 QJsonObject
    }
    else
    {
        // 处理 JSON 解析错误
    }
}
//读取类型
QString MessagePackage::Type()const
{
    return m_data["type"].toString();
}
//设置类型
void MessagePackage::setType(QString type)
{
    m_data.insert("type",type);
}
//读取字符数据
QString MessagePackage::getStringValue(const QString& key)const
{
    return m_data[key].toString();
}
//添加字符数据
void MessagePackage::addValue(QString key, QString value)
{
    m_data.insert(key,value);
}
//读取整形数据
int MessagePackage::getIntValue(QString key)const
{
    return m_data[key].toInt();
}
//添加整形数据
void MessagePackage::addValue(QString key, int value)
{
    m_data.insert(key,value);
}
//读取列表数据
QStringList MessagePackage::getListValue(const QString& key)const
{
    // 检查 key 是否存在于 m_data 中，并且对应的值是 QJsonArray 类型
    if (m_data.value(key).isArray()) {
        QJsonArray arr = m_data.value(key).toArray();
        QStringList list;
        for (const QJsonValue &value : arr) {
            // 确保数组中的每个值都是 QString 类型
            if (value.isString()) {
                list.append(value.toString());
            }
        }
        return list;
    }
    // 如果 key 不存在或值不是 QJsonArray 类型，返回一个空的 QStringList
    return QStringList();
}
//条件列表项
void MessagePackage::addValue(QString key, QStringList values)
{
    QJsonArray array;
    for(const QString& it:values)
    {
        array.append(it);
    }
    m_data.insert(key,array);
}
//文件方法
void MessagePackage::addValue(const QString &key, const QByteArray &fileData) {
    // 将文件内容转换为 Base64 编码
    QString base64Data = QString::fromLatin1(fileData.toBase64());

    // 将 Base64 编码的字符串添加到 JSON 对象中
    m_data.insert(key, QJsonValue(base64Data));
}

QByteArray MessagePackage::getFileValue(const QString& key) const {
    if (m_data.contains(key) && m_data.value(key).isString()) {
        QString base64Data = m_data.value(key).toString();

        // 将 Base64 编码的字符串转换回 QByteArray
        QByteArray base64EncodedData = base64Data.toUtf8(); // 将 QString 转换为 QByteArray
        QByteArray fileData = QByteArray::fromBase64(base64EncodedData); // 从 Base64 编码转换回原始数据

        return fileData;
    }
    // 如果 key 不存在或值不是字符串类型，返回空的 QByteArray
    return QByteArray();
}
//发送协议包
void MessagePackage::sendMsg(QTcpSocket *socket)
{
    //json转成QByteArray用于发送
    QJsonDocument doc(m_data);
    QByteArray array=doc.toJson();
    //先发送长度，防止读取粘包的问题
    qint64 size=array.size();
    qDebug()<<"pack size:"<<size;
    socket->write((char*)&size,4);
    //再发送数据
    socket->write(array);
}

void MessagePackage::recvMsg(QTcpSocket *socket)
{
    //先读取长度，保证不对多读
    int len=0;
    socket->read((char*)&len,4);
    //再读取数据
    QByteArray arr=socket->read(len);
    //将数据转成json对象保存再类中
    m_data=QJsonDocument::fromJson(arr).object();
}

