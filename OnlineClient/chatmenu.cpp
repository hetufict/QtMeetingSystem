#include "chatmenu.h"
#include "ui_chatmenu.h"
#include <QString>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
ChatMenu::ChatMenu(QWidget *parent,bool group)
    : QWidget(parent)
    , ui(new Ui::ChatMenu)
    ,objname("")
    ,group(group)
{
    ui->setupUi(this);
    // 自动调整列宽以适应内容
    ui->tableWidget_files->resizeColumnsToContents();
    this->hide();
}

void ChatMenu::setObjName(const QString& name)
{
    objname=name;
    if(group)//私聊对象是用户
    {
        QString getName=name;
        getName="group:"+name;
    }
    ui->label_objName->setText(name);
}
ChatMenu::~ChatMenu()
{
    delete ui;
}
void ChatMenu::addMsgText(const QString &msg)
{
    addMsgText(msg,false);
}

void ChatMenu::on_pb_sendMsg_clicked() {
    QString msg = ui->textEdit->toPlainText();
    if (!msg.isEmpty()) {
        ui->textEdit->clear();

        // 根据是群聊还是私聊，执行不同的操作
        if (group) {
            // 群聊消息处理
            addMsgText(msg, true); // 这里假设 addPrivateText 也适用于群聊消息的展示
            emit sendGroupChat(objname, msg); // 发送群聊消息信号
        } else {
            // 私聊消息处理
            addMsgText(msg, true); // 添加自己的消息到列表，并标记为右对齐
            emit sendPrivateChat(objname, msg); // 发送私聊消息信号
        }
    }
}
void ChatMenu::addMsgText(const QString &msg, bool isSelf) {
    QListWidgetItem *item = new QListWidgetItem(msg);
    if (isSelf) {
        // 如果是自己发送的消息，则设置对齐方式为右对齐
        item->setTextAlignment(Qt::AlignRight);
    } else {
        // 如果是接收的消息，则设置对齐方式为左对齐
        item->setTextAlignment(Qt::AlignLeft);
    }
    ui->listWidget_dialogView->addItem(item);
}

void ChatMenu::addFile(const QString &filename)
{

}

void ChatMenu::addGroupMembers(const QStringList &list)
{
    // 清空当前列表，确保只显示最新的群成员
    ui->listWidget_info->clear();

    // 遍历成员列表并添加到 QListWidget
    for (const QString &memberName : list) {
        ui->listWidget_info->addItem(memberName); // 添加每个成员到列表
    }
}

void ChatMenu::addFileList(const QStringList &filelist, const QStringList &senderlist) {
    // 清空现有列表项
    ui->tableWidget_files->clearContents();

    // 确保文件列表和发送者列表的大小相同
    if (filelist.size() != senderlist.size()) {
        qDebug() << "文件列表和发送者列表的大小不匹配";
        return;
    }
    // 设置 QTableWidget 的行数
    ui->tableWidget_files->setRowCount(filelist.size());

    // 遍历文件列表和发送者列表，填充数据到 QTableWidget
    for (int i = 0; i < filelist.size(); ++i) {
        // 创建新的 QTableWidgetItem 对象
        QTableWidgetItem *senderItem = new QTableWidgetItem(senderlist[i]);
        QTableWidgetItem *fileItem = new QTableWidgetItem(filelist[i]);

        // 将 QTableWidgetItem 对象添加到 QTableWidget
        ui->tableWidget_files->setItem(i, 0, senderItem); // 第一列：发送者
        ui->tableWidget_files->setItem(i, 1, fileItem); // 第二列：文件名
    }

    // 调整列宽以适应内容
    ui->tableWidget_files->resizeColumnsToContents();
}


void ChatMenu::on_pb_sendFile_clicked() {
    // 打开文件对话框让用户选择文件
    QString filePath = QFileDialog::getOpenFileName(this, tr("发送文件"), "", tr("All Files (*)"));
    // 检查用户是否选择了文件
    if (filePath.isEmpty()) {
        // 如果没有选择文件，直接返回
        qDebug()<<"empty";
        return;
    }
    // 发送信号给主窗口，传递文件路径、文件名和文件内容
    emit fileSent(objname, filePath,group);
}


//刷新获取群员信息
void ChatMenu::on_pb_flushgroupmember_clicked()
{
    emit flushGrroupMembers(objname);
}


void ChatMenu::on_pb_fllushFilelist_clicked()
{
    emit flushFileList(objname,group);
}


void ChatMenu::on_pb_downloadFile_clicked()
{
    // 获取当前选中的行
    int currentRow = ui->tableWidget_files->currentRow();

    if (currentRow >= 0) { // 确保有选中的行
        // 获取发送者名和文件名
        QTableWidgetItem *senderItem = ui->tableWidget_files->item(currentRow, 0); // 第一列：发送者
        QTableWidgetItem *fileItem = ui->tableWidget_files->item(currentRow, 1); // 第二列：文件名

        if (senderItem && fileItem) {
            QString senderName = senderItem->text();
            QString fileName = fileItem->text();

            // 发送信号，传递文件名和发送者名
            emit requestFile(fileName, senderName,objname,group);
        } else {
            // 如果某一项为空，可以提示用户
            QMessageBox::information(this, tr("错误"), tr("数据不完整，无法下载文件"));
        }
    } else {
        // 如果没有选中的行，可以提示用户
        QMessageBox::information(this, tr("提示"), tr("请选择一个文件"));
    }
}

