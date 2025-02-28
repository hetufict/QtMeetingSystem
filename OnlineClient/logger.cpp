#include "Logger.h"

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : currentLevel(Debug), outputToConsole(true) {
    outputToConsole = true;
}

Logger::~Logger() {
    if (logFile.isOpen()) {
        logFile.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::setOutputFile(const QString& filePath) {
    if (logFile.isOpen()) {
        logFile.close();
    }
    logFile.setFileName(filePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << filePath;
    }
    logStream.setDevice(&logFile);
}

void Logger::log(LogLevel level, const QString& message) {
    if (level < currentLevel) {
        return;
    }
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString logMessage = QString("[%1] [%2] %3").arg(timestamp, LogLevelToString(level), message);
    writeLog(logMessage);
}

void Logger::writeLog(const QString& message) {
    if (logFile.isOpen()) {
        logStream <<message<<Qt::endl;
        logStream.flush();
    }
}

QString Logger::LogLevelToString(LogLevel level) {
    switch (level) {
    case Logger::Debug: return "DEBUG";
    case Logger::Info: return "INFO";
    case Logger::Warning: return "WARNING";
    case Logger::Error: return "ERROR";
    default: return "UNKNOWN";
    }
}

QString Logger::getCurrentTime()
{
    QDateTime now = QDateTime::currentDateTime();
    return now.toString("yyyy-MM-dd hh:mm:ss.zzz");
}
