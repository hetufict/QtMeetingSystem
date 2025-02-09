#include "clienttask.h"

ClientTask::ClientTask(QObject *parent, QTcpSocket *socket)
    : QObject{parent},QRunnable(),socket(socket)
{
    setAutoDelete(true);
}

void ClientTask::run()
{

    if(socket)
    {
        while(socket->bytesAvailable()>0)
        {
            MessagePackage pack(nullptr,0); //生成数据包对象
            pack.recvMsg(socket); //读取数据
            //数据处理
            handler(pack);
        }
    }
}

void ClientTask::handler(MessagePackage pack)
{
    QString type = pack.Type(); //获取请求类型
    if(type == "user login")
    {
        loginHandler(pack);
    }
}

void ClientTask::loginHandler(MessagePackage pack)
{
    //获取用户信息
    QString name = pack.getStringValue("name");
    QString pswd = pack.getStringValue("pswd");
    //检测登录结果
    int ret = DBHelper::getInstance()->userLogin(name,pswd);
    qDebug()<<"name:"<<pack.getStringValue("name")<<" pswd:"<<pack.getStringValue("pswd");
    //记录结果
    pack.addValue("result",ret);
    if(ret)
    {
    }

    //反馈给客户端
    pack.sendMsg(socket);
}
