#include "meetiingmenu.h"
#include "ui_meetiingmenu.h"

MeetiingMenu::MeetiingMenu(QWidget *parent,QTcpSocket* socket,QString username)
    : QWidget(parent)
    , ui(new Ui::MeetiingMenu)
    ,socket(socket)
    ,username(username)
{
    ui->setupUi(this);
}

MeetiingMenu::~MeetiingMenu()
{
    delete ui;
}

void MeetiingMenu::onCreateMeeting(const MessagePackage &pack)
{

}

void MeetiingMenu::oninvited(const QString &name, const QString &meetName, int &meetingID)
{

}

void MeetiingMenu::onInviteMeetingRespond(const MessagePackage &pack)
{

}

void MeetiingMenu::onGetMeetingInvitation(const MessagePackage &pack)
{

}

void MeetiingMenu::onJoinMeeting(const MessagePackage &pack)
{

}

void MeetiingMenu::onMeetingRoomClose()
{

}

void MeetiingMenu::onMeetingClosed(const MessagePackage &pack)
{

}

void MeetiingMenu::onMeetingExit(const MessagePackage &pack)
{

}

void MeetiingMenu::onMeetingMembersList(const MessagePackage &pack)
{

}
