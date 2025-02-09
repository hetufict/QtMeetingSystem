#ifndef CREATEMEETINGDIALOG_H
#define CREATEMEETINGDIALOG_H

#include <QDialog>
#include <QString>
namespace Ui {
class CreateMeetingDialog;
}

class CreateMeetingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateMeetingDialog(QWidget *parent = nullptr);
    ~CreateMeetingDialog();
    QString getName()const;
    void setDefaultName(QString user);
    static QString toCreateMeeting(QString user,QWidget* parent=nullptr);
private:
    Ui::CreateMeetingDialog *ui;
};

#endif // CREATEMEETINGDIALOG_H
