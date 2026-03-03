#include "Logger.h"
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <iostream>

Q_LOGGING_CATEGORY(tvtest, "tvtest")

LogLevel Logger::s_logLevel = LogLevel::Info;
QFile Logger::s_logFile;
QTextStream Logger::s_logStream;
QMutex Logger::s_mutex;
bool Logger::s_initialized = false;

void Logger::initialize()
{
    QMutexLocker locker(&s_mutex);
    
    if (s_initialized) {
        return;
    }
    
    // Set default log level based on build type
#ifdef QT_DEBUG
    s_logLevel = LogLevel::Trace;
#else
    s_logLevel = LogLevel::Info;
#endif
    
    // Setup log file path
    QString logDir;
#ifdef Q_OS_ANDROID
    logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
#else
    logDir = QCoreApplication::applicationDirPath() + "/logs";
#endif
    
    QDir().mkpath(logDir);
    QString logFilePath = logDir + "/tvtest_debug.log";
    setLogFile(logFilePath);
    
    // Install Qt message handler
    qInstallMessageHandler(messageHandler);
    
    s_initialized = true;
    
    info("Logger: Logging system initialized");
    info("Logger: Log file: " + logFilePath);
    info("Logger: Log level: " + QString::number(static_cast<int>(s_logLevel)));
}

void Logger::shutdown()
{
    QMutexLocker locker(&s_mutex);
    
    if (!s_initialized) {
        return;
    }
    
    info("Logger: Shutting down logging system");
    
    if (s_logFile.isOpen()) {
        s_logStream.flush();
        s_logFile.close();
    }
    
    qInstallMessageHandler(nullptr);
    s_initialized = false;
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&s_mutex);
    s_logLevel = level;
}

void Logger::setLogFile(const QString &filePath)
{
    QMutexLocker locker(&s_mutex);
    
    if (s_logFile.isOpen()) {
        s_logStream.flush();
        s_logFile.close();
    }
    
    s_logFile.setFileName(filePath);
    if (s_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        s_logStream.setDevice(&s_logFile);
        s_logStream.setEncoding(QStringConverter::Utf8);
    }
}

void Logger::trace(const QString &message)
{
    writeLog(LogLevel::Trace, message);
}

void Logger::debug(const QString &message)
{
    writeLog(LogLevel::Debug, message);
}

void Logger::info(const QString &message)
{
    writeLog(LogLevel::Info, message);
}

void Logger::warning(const QString &message)
{
    writeLog(LogLevel::Warning, message);
}

void Logger::error(const QString &message)
{
    writeLog(LogLevel::Error, message);
}

void Logger::fatal(const QString &message)
{
    writeLog(LogLevel::Fatal, message);
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    LogLevel level;
    
    switch (type) {
    case QtDebugMsg:
        level = LogLevel::Debug;
        break;
    case QtInfoMsg:
        level = LogLevel::Info;
        break;
    case QtWarningMsg:
        level = LogLevel::Warning;
        break;
    case QtCriticalMsg:
        level = LogLevel::Error;
        break;
    case QtFatalMsg:
        level = LogLevel::Fatal;
        break;
    }
    
    QString contextInfo;
    if (context.file) {
        contextInfo = QString("[%1:%2]").arg(context.file).arg(context.line);
    }
    
    writeLog(level, contextInfo + " " + message);
}

void Logger::writeLog(LogLevel level, const QString &message)
{
    QMutexLocker locker(&s_mutex);
    
    if (!s_initialized || level < s_logLevel) {
        return;
    }
    
    QString formattedMessage = formatMessage(level, message);
    
    // Write to console
    std::cout << formattedMessage.toStdString() << std::endl;
    
    // Write to file
    if (s_logFile.isOpen()) {
        s_logStream << formattedMessage << Qt::endl;
        s_logStream.flush();
        
        // Check file size and rotate if necessary
        if (s_logFile.size() > MAX_LOG_FILE_SIZE) {
            rotateLogFile();
        }
    }
}

QString Logger::formatMessage(LogLevel level, const QString &message)
{
    QString levelStr;
    switch (level) {
    case LogLevel::Trace:   levelStr = "TRACE"; break;
    case LogLevel::Debug:   levelStr = "DEBUG"; break;
    case LogLevel::Info:    levelStr = "INFO "; break;
    case LogLevel::Warning: levelStr = "WARN "; break;
    case LogLevel::Error:   levelStr = "ERROR"; break;
    case LogLevel::Fatal:   levelStr = "FATAL"; break;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    return QString("[%1] %2: %3").arg(timestamp, levelStr, message);
}

void Logger::rotateLogFile()
{
    if (!s_logFile.isOpen()) {
        return;
    }
    
    QString basePath = s_logFile.fileName();
    s_logStream.flush();
    s_logFile.close();
    
    // Rotate existing files
    for (int i = LOG_FILE_ROTATION - 1; i > 0; --i) {
        QString oldFile = basePath + QString(".%1").arg(i);
        QString newFile = basePath + QString(".%1").arg(i + 1);
        
        if (QFile::exists(newFile)) {
            QFile::remove(newFile);
        }
        
        if (QFile::exists(oldFile)) {
            QFile::rename(oldFile, newFile);
        }
    }
    
    // Move current file to .1
    QString rotatedFile = basePath + ".1";
    if (QFile::exists(rotatedFile)) {
        QFile::remove(rotatedFile);
    }
    QFile::rename(basePath, rotatedFile);
    
    // Reopen new log file
    if (s_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        s_logStream.setDevice(&s_logFile);
    }
}