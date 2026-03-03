#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QQueue>
#include <QtCore/QMutex>

enum class StreamType {
    Unknown,
    HLS,        // HTTP Live Streaming
    DASH,       // Dynamic Adaptive Streaming over HTTP
    UDP,        // UDP Multicast
    HTTP        // Direct HTTP stream
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Error
};

struct StreamInfo {
    QString url;
    StreamType type;
    QString contentType;
    qint64 bitrate;
    bool isLive;
};

class NetworkStreamWorker;

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    bool initialize();
    void shutdown();

    // Stream control
    bool openNetworkStream(const QString &url);
    bool openUDPStream(const QString &url);
    void closeStream();
    
    // Stream info
    StreamInfo getStreamInfo() const { return m_streamInfo; }
    ConnectionState getConnectionState() const { return m_connectionState; }
    
    // Network status
    qint64 getBytesReceived() const { return m_bytesReceived; }
    double getDownloadRate() const { return m_downloadRate; }
    int getBufferLevel() const;
    
    // Connection settings
    void setConnectionTimeout(int timeoutMs);
    void setRetryAttempts(int maxRetries);
    void setBufferSize(int bufferSize);

signals:
    void streamReady(const QString &streamUrl);
    void dataReceived(const QByteArray &data);
    void connectionStateChanged(ConnectionState state);
    void errorOccurred(const QString &error);
    void statisticsUpdated(qint64 bytesReceived, double downloadRate);

public slots:
    void reconnect();

private slots:
    void onNetworkReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError error);
    void onUdpDataReceived();
    void onTcpDataReceived();
    void updateStatistics();
    void checkConnection();

private:
    void setConnectionState(ConnectionState state);
    StreamType detectStreamType(const QString &url);
    bool processHLSPlaylist(const QByteArray &data);
    bool processDASHManifest(const QByteArray &data);
    void setupUDPConnection(const QString &url);
    void setupHTTPConnection(const QString &url);
    void startStatisticsTimer();
    void stopStatisticsTimer();
    
    // Network components
    QNetworkAccessManager *m_networkAccess;
    QNetworkReply *m_currentReply;
    QUdpSocket *m_udpSocket;
    QTcpSocket *m_tcpSocket;
    
    // Stream worker thread
    NetworkStreamWorker *m_streamWorker;
    QThread *m_workerThread;
    
    // Stream information
    StreamInfo m_streamInfo;
    ConnectionState m_connectionState;
    QString m_currentUrl;
    
    // Buffer management
    QQueue<QByteArray> m_dataBuffer;
    mutable QMutex m_bufferMutex;
    int m_maxBufferSize;
    int m_currentBufferSize;
    
    // Statistics
    qint64 m_bytesReceived;
    qint64 m_lastBytesReceived;
    double m_downloadRate;
    QTimer *m_statisticsTimer;
    QTimer *m_connectionTimer;
    
    // Connection settings
    int m_connectionTimeout;
    int m_maxRetryAttempts;
    int m_currentRetryAttempt;
    
    // Android specific optimizations
    static const int DEFAULT_BUFFER_SIZE = 1024 * 1024; // 1MB for Android
    static const int MAX_BUFFER_PACKETS = 100;
};

// Worker class for threaded network operations
class NetworkStreamWorker : public QObject
{
    Q_OBJECT

public:
    explicit NetworkStreamWorker(QObject *parent = nullptr);
    
public slots:
    void processStream(const QString &url);
    void stopProcessing();

signals:
    void dataReady(const QByteArray &data);
    void errorOccurred(const QString &error);

private:
    void processUDPStream(const QString &url);
    void processHTTPStream(const QString &url);
    
    bool m_isProcessing;
    QMutex m_processingMutex;
};

#endif // NETWORKMANAGER_H