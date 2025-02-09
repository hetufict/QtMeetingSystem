#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QObject>
#include <QAudioInput>
#include <QUdpSocket>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QMediaDevices>
#include <QAudioSource>
class AudioCapture : public QObject
{
    Q_OBJECT
public:
    explicit AudioCapture(int port,QObject *parent = nullptr);
    ~AudioCapture();
    void startSpeak();
    void stopSpeak();
signals:

private:
    QAudioSource* audio;
    QUdpSocket* socket;
    int port;
};

#endif // AUDIOCAPTURE_H
