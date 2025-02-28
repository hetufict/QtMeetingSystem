#include <QMessageBox>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QTransform>
#include "camerahandler.h"
#include "logger.h"
CameraHandler::CameraHandler(QObject *parent)
    : QObject{parent}
    , cap(0) // 打开默认摄像头
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CameraHandler::updateCameraFrame);
    timer->start(60); // 更新间隔30ms
    if (!cap.isOpened()) {
        qDebug() << "无法打开摄像头";
        LOG(Logger::Error,"open cameral failed");
    }

}

void CameraHandler::updateCameraFrame()
{
    static int i=0;
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) {
        //QMessageBox::critical(this, "错误", "无法从摄像头读取数据");
        LOG(Logger::Error,"can not read data from cameral");
        return;
    }
    QImage img = MatToQImage(frame);

    //旋转图像180
    img=img.scaled(160,160);
    emit getImage(img );
}

QImage CameraHandler::MatToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC1) {
        QImage image(mat.cols, mat.rows, QImage::Format_Grayscale8);
        uchar *ptr = image.bits();
        for (int i = 0; i < mat.rows; ++i) {
            memcpy(ptr, mat.ptr(i), mat.cols);
            ptr += image.bytesPerLine();
        }

        return image;
    } else if (mat.type() == CV_8UC3) {
        const uchar *qImgBuf = (const uchar*)mat.data;
        QImage img(qImgBuf, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return img.rgbSwapped();
    } else {
        qDebug() << "cv::Mat image type not handled in switch:" << mat.type();
        LOG(Logger::Warning,"cv::Mat image type not handled in switch:"+mat.type());
        return QImage();
    }
}
