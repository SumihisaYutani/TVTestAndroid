#include "Logger.h"
#include <QCoreApplication>
#include <QDebug>

Logger *Logger::s_instance = nullptr;

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_logFile(nullptr)
    , m_logStream(nullptr)
{
}

Logger::~Logger()
{
    if (m_logStream) {
        m_logStream->flush();
        delete m_logStream;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
}

Logger* Logger::instance()
{
    if (!s_instance) {
        s_instance = new Logger();
    }
    return s_instance;
}

bool Logger::initialize(const QString &logDir)
{
    QMutexLocker locker(&m_mutex);

    // 既に初期化されている場合はクローズ
    if (m_logStream) {
        delete m_logStream;
        m_logStream = nullptr;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }

    // ログディレクトリの決定
    QString logDirPath = logDir;
    if (logDirPath.isEmpty()) {
        // 実行フォルダの下のlogsディレクトリ
        logDirPath = QCoreApplication::applicationDirPath() + "/logs";
    }

    // ログディレクトリ作成
    QDir dir;
    if (!dir.mkpath(logDirPath)) {
        return false;
    }

    // ログファイル名（タイムスタンプ付き）
    QDateTime now = QDateTime::currentDateTime();
    QString fileName = QString("tvtest_%1.log").arg(now.toString("yyyyMMdd_hhmmss"));
    m_logFilePath = logDirPath + "/" + fileName;

    // ログファイルオープン
    m_logFile = new QFile(m_logFilePath);
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        delete m_logFile;
        m_logFile = nullptr;
        return false;
    }

    m_logStream = new QTextStream(m_logFile);
    
    // 初期ログ出力
    try {
        // 直接ストリームに書き込み
        *m_logStream << "=== TVTest Android ログ開始 ===" << Qt::endl;
        *m_logStream << QString("ログファイル: %1").arg(m_logFilePath) << Qt::endl;
        *m_logStream << QString("開始時刻: %1").arg(now.toString("yyyy-MM-dd hh:mm:ss")) << Qt::endl;
        *m_logStream << QString("アプリケーションバージョン: %1").arg(QCoreApplication::applicationVersion()) << Qt::endl;
        m_logStream->flush();
    } catch (...) {
        return false;
    }

    return true;
}

void Logger::writeLog(LogLevel level, const QString &message, const char *file, int line)
{
    QMutexLocker locker(&m_mutex);

    if (!m_logStream || !m_logFile) {
        // ログが初期化されていない場合、コンソールのみに出力
        return;
    }

    // タイムスタンプ
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    
    // ログレベル文字列
    QString levelStr = logLevelToString(level);
    
    // ファイル名・行番号（指定されている場合）
    QString location;
    if (file && line > 0) {
        location = QString(" [%1:%2]").arg(extractFileName(file)).arg(line);
    }

    // ログ行構築
    QString logLine = QString("[%1] %2%3: %4")
                     .arg(timestamp)
                     .arg(levelStr)
                     .arg(location)
                     .arg(message);

    // ファイルのみに出力（統一）
    try {
        *m_logStream << logLine << Qt::endl;
    } catch (...) {
        return;
    }
    
    // 重要なレベルは即座にフラッシュ
    if (level >= Warning) {
        m_logStream->flush();
    }
    
    // コンソール出力を無効化してファイルログに統一
    // UI側は必要に応じてログファイルを監視
}

QString Logger::getLogFilePath() const
{
    return m_logFilePath;
}

void Logger::flush()
{
    QMutexLocker locker(&m_mutex);
    if (m_logStream) {
        m_logStream->flush();
    }
}

QString Logger::logLevelToString(LogLevel level)
{
    switch (level) {
        case Debug:    return "DEBUG";
        case Info:     return "INFO ";
        case Warning:  return "WARN ";
        case Critical: return "CRIT ";
        case Fatal:    return "FATAL";
        default:       return "UNKNOWN";
    }
}

QString Logger::extractFileName(const char *filePath)
{
    if (!filePath) return QString();
    
    QString path(filePath);
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash == -1) {
        lastSlash = path.lastIndexOf('\\');
    }
    return (lastSlash != -1) ? path.mid(lastSlash + 1) : path;
}