#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>

class Logger : public QObject {
    Q_OBJECT

public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };

    static Logger& getInstance();
    void setLogLevel(LogLevel level);
    void setOutputFile(const QString& filePath);
    void log(LogLevel level, const QString& message);

private:
    Logger();
    ~Logger();

    LogLevel currentLevel;
    QFile logFile;
    QTextStream logStream;
    bool outputToConsole;

    void writeLog(const QString& message);
    static QString LogLevelToString(LogLevel level);
    static QString getCurrentTime();
};

#define LOG(level, message) Logger::getInstance().log(level, message)

#endif // LOGGER_H
