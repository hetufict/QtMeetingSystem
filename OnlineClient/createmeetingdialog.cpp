#include "createmeetingdialog.h"
#include "ui_createmeetingdialog.h"

CreateMeetingDialog::CreateMeetingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CreateMeetingDialog)
{
    ui->setupUi(this);
}

CreateMeetingDialog::~CreateMeetingDialog()
{
    delete ui;
}

QString CreateMeetingDialog::getName()const
{
    return ui->LE_Meetingname->text();
}

void CreateMeetingDialog::setDefaultName(QString user)
{
    ui->LE_Meetingname->setText(user+"的会议");
}

QString CreateMeetingDialog::toCreateMeeting(QString user, QWidget *parent)
{
    QString name;
    CreateMeetingDialog dialog(parent);//新建对话框
    dialog.setDefaultName(user);//设置默认会议名

    int ret=dialog.exec();//运行，等待对话框结果

    if(ret==QDialog::Accepted)//点击确定
    {
        name=dialog.getName();
    }
    return name;
}
