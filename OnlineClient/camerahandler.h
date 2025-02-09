#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <QObject>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <QImage>
class CameraHandler : public QObject
{
    Q_OBJECT
public:
    explicit CameraHandler(QObject *parent = nullptr);
private slots:
    void updateCameraFrame();
signals:
    void getImage(QImage img);
private:
    QTimer *timer;//定时采集摄像头数据
    cv::VideoCapture cap;//opencv提供打开摄像头

    QImage MatToQImage(const cv::Mat &mat);

};

#endif // CAMERAHANDLER_H
