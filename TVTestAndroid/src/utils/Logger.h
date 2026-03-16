#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDir>
#include <QStandardPaths>

/**
 * @brief ファイルログ出力クラス
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief ログレベル
     */
    enum LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Critical = 3,
        Fatal = 4
    };

    /**
     * @brief シングルトンインスタンス取得
     */
    static Logger* instance();

    /**
     * @brief ログ初期化
     * @param logDir ログディレクトリ（空の場合はデフォルト）
     * @return 初期化成功時true
     */
    bool initialize(const QString &logDir = QString());

    /**
     * @brief ログ出力
     * @param level ログレベル
     * @param message メッセージ
     * @param file ファイル名（__FILE__）
     * @param line 行番号（__LINE__）
     */
    void writeLog(LogLevel level, const QString &message, const char *file = nullptr, int line = 0);

    /**
     * @brief ログファイルパス取得
     */
    QString getLogFilePath() const;

public slots:
    /**
     * @brief ログファイルフラッシュ
     */
    void flush();

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    QString logLevelToString(LogLevel level);
    QString extractFileName(const char *filePath);

private:
    static Logger *s_instance;
    QFile *m_logFile;
    QTextStream *m_logStream;
    QMutex m_mutex;
    QString m_logFilePath;
};

// 便利マクロ
#define LOG_DEBUG(msg) Logger::instance()->writeLog(Logger::Debug, msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::instance()->writeLog(Logger::Info, msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::instance()->writeLog(Logger::Warning, msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg) Logger::instance()->writeLog(Logger::Critical, msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) Logger::instance()->writeLog(Logger::Fatal, msg, __FILE__, __LINE__)

#endif // LOGGER_H