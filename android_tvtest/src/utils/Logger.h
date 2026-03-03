#ifndef LOGGER_H
#define LOGGER_H

#include <QtCore/QString>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QMutex>

Q_DECLARE_LOGGING_CATEGORY(tvtest)

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5
};

class Logger
{
public:
    static void initialize();
    static void shutdown();
    
    static void setLogLevel(LogLevel level);
    static void setLogFile(const QString &filePath);
    
    static void trace(const QString &message);
    static void debug(const QString &message);
    static void info(const QString &message);
    static void warning(const QString &message);
    static void error(const QString &message);
    static void fatal(const QString &message);
    
    // Qt logging integration
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);

private:
    Logger() = delete;
    
    static void writeLog(LogLevel level, const QString &message);
    static QString formatMessage(LogLevel level, const QString &message);
    static void rotateLogFile();
    
    static LogLevel s_logLevel;
    static QFile s_logFile;
    static QTextStream s_logStream;
    static QMutex s_mutex;
    static bool s_initialized;
    
    // Log file rotation
    static const qint64 MAX_LOG_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    static const int LOG_FILE_ROTATION = 5; // Keep 5 generations
};

#endif // LOGGER_H