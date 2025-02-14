#include "audiocapture.h"
#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaFormat>
AudioCapture::AudioCapture(int port, QObject *parent)
    : QObject(parent), socket(new QUdpSocket(this)), port(port) {

    // 设置音频格式
    QAudioFormat format;
    //QMediaFormat format;
    format.setSampleRate(8000);  // 设置采样率为 8000 Hz
    format.setChannelCount(1);  // 设置单声道
    format.setSampleFormat(QAudioFormat::UInt8);  // 设置样本格式为 8 位有符号整数

    QAudioDevice info=QMediaDevices::defaultAudioInput();
    if(!info.isFormatSupported(format)){
        qDebug()<<"不支持默认格式";
    }
    audio=new QAudioSource(format,this);
    //audio->start();
}

AudioCapture::~AudioCapture()
{

}

void AudioCapture::startSpeak()
{
    //打开麦克风

}
