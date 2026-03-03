#include "NetworkManager.h"
#include "../utils/Logger.h"

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QHostAddress>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_networkAccess(nullptr)
    , m_currentReply(nullptr)
    , m_udpSocket(nullptr)
    , m_tcpSocket(nullptr)
    , m_streamWorker(nullptr)
    , m_workerThread(nullptr)
    , m_connectionState(ConnectionState::Disconnected)
    , m_maxBufferSize(DEFAULT_BUFFER_SIZE)
    , m_currentBufferSize(0)
    , m_bytesReceived(0)
    , m_lastBytesReceived(0)
    , m_downloadRate(0.0)
    , m_statisticsTimer(nullptr)
    , m_connectionTimer(nullptr)
    , m_connectionTimeout(30000) // 30 seconds
    , m_maxRetryAttempts(3)
    , m_currentRetryAttempt(0)
{
    Logger::info("NetworkManager: Initializing...");
    
    m_networkAccess = new QNetworkAccessManager(this);
    m_statisticsTimer = new QTimer(this);
    m_connectionTimer = new QTimer(this);
    
    connect(m_statisticsTimer, &QTimer::timeout, this, &NetworkManager::updateStatistics);
    connect(m_connectionTimer, &QTimer::timeout, this, &NetworkManager::checkConnection);
    
    // Initialize stream info
    m_streamInfo.type = StreamType::Unknown;
    m_streamInfo.isLive = false;
    m_streamInfo.bitrate = 0;
    
    Logger::info("NetworkManager: Initialization complete");
}

NetworkManager::~NetworkManager()
{
    Logger::info("NetworkManager: Shutting down...");
    shutdown();
}

bool NetworkManager::initialize()
{
    Logger::info("NetworkManager: Starting initialization...");
    
    // Setup worker thread for intensive network operations
    m_streamWorker = new NetworkStreamWorker();
    m_workerThread = new QThread(this);
    m_streamWorker->moveToThread(m_workerThread);
    
    connect(m_workerThread, &QThread::finished, m_streamWorker, &QObject::deleteLater);
    connect(m_streamWorker, &NetworkStreamWorker::dataReady, this, &NetworkManager::dataReceived);
    connect(m_streamWorker, &NetworkStreamWorker::errorOccurred, this, &NetworkManager::errorOccurred);
    
    m_workerThread->start();
    
    Logger::info("NetworkManager: Initialization successful");
    return true;
}

void NetworkManager::shutdown()
{
    Logger::info("NetworkManager: Shutting down...");
    
    closeStream();
    
    // Stop timers
    if (m_statisticsTimer && m_statisticsTimer->isActive()) {
        m_statisticsTimer->stop();
    }
    
    if (m_connectionTimer && m_connectionTimer->isActive()) {
        m_connectionTimer->stop();
    }
    
    // Shutdown worker thread
    if (m_workerThread && m_workerThread->isRunning()) {
        if (m_streamWorker) {
            QMetaObject::invokeMethod(m_streamWorker, "stopProcessing", Qt::QueuedConnection);
        }
        m_workerThread->quit();
        m_workerThread->wait(5000);
    }
    
    setConnectionState(ConnectionState::Disconnected);
    
    Logger::info("NetworkManager: Shutdown complete");
}

bool NetworkManager::openNetworkStream(const QString &url)
{
    Logger::info("NetworkManager: Opening network stream: " + url);
    
    if (url.isEmpty()) {
        Logger::error("NetworkManager: Empty URL provided");
        return false;
    }
    
    // Close any existing stream
    closeStream();
    
    m_currentUrl = url;
    m_currentRetryAttempt = 0;
    
    // Detect stream type
    m_streamInfo.url = url;
    m_streamInfo.type = detectStreamType(url);
    
    setConnectionState(ConnectionState::Connecting);
    
    switch (m_streamInfo.type) {
    case StreamType::HLS:
    case StreamType::DASH:
    case StreamType::HTTP:
        setupHTTPConnection(url);
        break;
    case StreamType::UDP:
        setupUDPConnection(url);
        break;
    default:
        Logger::error("NetworkManager: Unknown stream type for URL: " + url);
        setConnectionState(ConnectionState::Error);
        return false;
    }
    
    // Start connection timeout
    m_connectionTimer->start(m_connectionTimeout);
    
    return true;
}

bool NetworkManager::openUDPStream(const QString &url)
{
    Logger::info("NetworkManager: Opening UDP stream: " + url);
    
    m_streamInfo.type = StreamType::UDP;
    return openNetworkStream(url);
}

void NetworkManager::closeStream()
{
    Logger::info("NetworkManager: Closing stream");
    
    // Stop timers
    stopStatisticsTimer();
    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }
    
    // Close network connections
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    if (m_udpSocket) {
        m_udpSocket->close();
        m_udpSocket->deleteLater();
        m_udpSocket = nullptr;
    }
    
    if (m_tcpSocket) {
        m_tcpSocket->close();
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
    }
    
    // Stop worker
    if (m_streamWorker) {
        QMetaObject::invokeMethod(m_streamWorker, "stopProcessing", Qt::QueuedConnection);
    }
    
    // Clear buffer
    QMutexLocker locker(&m_bufferMutex);
    m_dataBuffer.clear();
    m_currentBufferSize = 0;
    
    // Reset statistics
    m_bytesReceived = 0;
    m_lastBytesReceived = 0;
    m_downloadRate = 0.0;
    
    setConnectionState(ConnectionState::Disconnected);
    
    Logger::info("NetworkManager: Stream closed");
}

int NetworkManager::getBufferLevel() const
{
    QMutexLocker locker(&m_bufferMutex);
    return (m_currentBufferSize * 100) / m_maxBufferSize;
}

void NetworkManager::setConnectionTimeout(int timeoutMs)
{
    m_connectionTimeout = timeoutMs;
    Logger::info("NetworkManager: Connection timeout set to " + QString::number(timeoutMs) + "ms");
}

void NetworkManager::setRetryAttempts(int maxRetries)
{
    m_maxRetryAttempts = maxRetries;
    Logger::info("NetworkManager: Max retry attempts set to " + QString::number(maxRetries));
}

void NetworkManager::setBufferSize(int bufferSize)
{
    m_maxBufferSize = bufferSize;
    Logger::info("NetworkManager: Buffer size set to " + QString::number(bufferSize) + " bytes");
}

void NetworkManager::reconnect()
{
    Logger::info("NetworkManager: Attempting to reconnect...");
    
    if (m_currentRetryAttempt >= m_maxRetryAttempts) {
        Logger::error("NetworkManager: Maximum retry attempts exceeded");
        emit errorOccurred("Connection failed after " + QString::number(m_maxRetryAttempts) + " attempts");
        setConnectionState(ConnectionState::Error);
        return;
    }
    
    m_currentRetryAttempt++;
    Logger::info("NetworkManager: Retry attempt " + QString::number(m_currentRetryAttempt));
    
    openNetworkStream(m_currentUrl);
}

void NetworkManager::onNetworkReplyFinished()
{
    Logger::info("NetworkManager: Network reply finished");
    
    if (!m_currentReply) {
        return;
    }
    
    if (m_currentReply->error() != QNetworkReply::NoError) {
        Logger::error("NetworkManager: Network error: " + m_currentReply->errorString());
        emit errorOccurred(m_currentReply->errorString());
        setConnectionState(ConnectionState::Error);
        return;
    }
    
    QByteArray data = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
    // Process data based on stream type
    switch (m_streamInfo.type) {
    case StreamType::HLS:
        if (processHLSPlaylist(data)) {
            setConnectionState(ConnectionState::Connected);
            startStatisticsTimer();
        }
        break;
    case StreamType::DASH:
        if (processDASHManifest(data)) {
            setConnectionState(ConnectionState::Connected);
            startStatisticsTimer();
        }
        break;
    case StreamType::HTTP:
        // Direct stream data
        emit dataReceived(data);
        m_bytesReceived += data.size();
        setConnectionState(ConnectionState::Connected);
        startStatisticsTimer();
        break;
    default:
        Logger::warning("NetworkManager: Unhandled stream type in reply");
        break;
    }
    
    m_connectionTimer->stop();
}

void NetworkManager::onNetworkError(QNetworkReply::NetworkError error)
{
    Logger::error("NetworkManager: Network error code: " + QString::number(static_cast<int>(error)));
    
    if (m_currentRetryAttempt < m_maxRetryAttempts) {
        QTimer::singleShot(2000, this, &NetworkManager::reconnect); // Retry after 2 seconds
    } else {
        setConnectionState(ConnectionState::Error);
    }
}

void NetworkManager::onUdpDataReceived()
{
    if (!m_udpSocket) {
        return;
    }
    
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(m_udpSocket->pendingDatagramSize());
        
        QHostAddress sender;
        quint16 senderPort;
        
        qint64 size = m_udpSocket->readDatagram(data.data(), data.size(), &sender, &senderPort);
        
        if (size > 0) {
            data.resize(size);
            emit dataReceived(data);
            m_bytesReceived += size;
            
            // Buffer management for UDP
            QMutexLocker locker(&m_bufferMutex);
            if (m_currentBufferSize < m_maxBufferSize) {
                m_dataBuffer.enqueue(data);
                m_currentBufferSize += size;
            }
        }
    }
}

void NetworkManager::onTcpDataReceived()
{
    if (!m_tcpSocket) {
        return;
    }
    
    QByteArray data = m_tcpSocket->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
        m_bytesReceived += data.size();
    }
}

void NetworkManager::updateStatistics()
{
    qint64 bytesDiff = m_bytesReceived - m_lastBytesReceived;
    m_downloadRate = bytesDiff / 1.0; // bytes per second
    m_lastBytesReceived = m_bytesReceived;
    
    emit statisticsUpdated(m_bytesReceived, m_downloadRate);
    
    // Convert to more readable units for logging
    double rateMbps = (m_downloadRate * 8.0) / (1024.0 * 1024.0); // Convert to Mbps
    
    if (rateMbps > 0.1) { // Only log if there's significant data flow
        Logger::info("NetworkManager: Download rate: " + QString::number(rateMbps, 'f', 2) + " Mbps, " +
                    "Total: " + QString::number(m_bytesReceived / 1024) + " KB");
    }
}

void NetworkManager::checkConnection()
{
    Logger::warning("NetworkManager: Connection timeout");
    
    if (m_connectionState == ConnectionState::Connecting) {
        closeStream();
        
        if (m_currentRetryAttempt < m_maxRetryAttempts) {
            reconnect();
        } else {
            emit errorOccurred("Connection timeout");
            setConnectionState(ConnectionState::Error);
        }
    }
}

void NetworkManager::setConnectionState(ConnectionState state)
{
    if (m_connectionState != state) {
        Logger::info("NetworkManager: Connection state: " + QString::number(static_cast<int>(m_connectionState)) +
                    " -> " + QString::number(static_cast<int>(state)));
        m_connectionState = state;
        emit connectionStateChanged(state);
    }
}

StreamType NetworkManager::detectStreamType(const QString &url)
{
    QUrl qurl(url);
    QString path = qurl.path().toLower();
    
    if (url.startsWith("udp://")) {
        return StreamType::UDP;
    } else if (path.endsWith(".m3u8")) {
        return StreamType::HLS;
    } else if (path.endsWith(".mpd")) {
        return StreamType::DASH;
    } else if (url.startsWith("http")) {
        return StreamType::HTTP;
    }
    
    return StreamType::Unknown;
}

bool NetworkManager::processHLSPlaylist(const QByteArray &data)
{
    Logger::info("NetworkManager: Processing HLS playlist");
    
    QString playlist = QString::fromUtf8(data);
    
    // Simple HLS parsing - find stream URLs
    QRegularExpression streamRegex(R"(^(?!#).*\.ts$)", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator matches = streamRegex.globalMatch(playlist);
    
    if (matches.hasNext()) {
        // Found TS segments, start processing
        m_streamInfo.isLive = playlist.contains("#EXT-X-PLAYLIST-TYPE:VOD") ? false : true;
        emit streamReady(m_streamInfo.url);
        return true;
    }
    
    Logger::warning("NetworkManager: No valid segments found in HLS playlist");
    return false;
}

bool NetworkManager::processDASHManifest(const QByteArray &data)
{
    Logger::info("NetworkManager: Processing DASH manifest");
    
    // Basic DASH processing - this would need more sophisticated parsing
    QString manifest = QString::fromUtf8(data);
    
    if (manifest.contains("MPD") && manifest.contains("AdaptationSet")) {
        m_streamInfo.isLive = manifest.contains("type=\"dynamic\"");
        emit streamReady(m_streamInfo.url);
        return true;
    }
    
    Logger::warning("NetworkManager: Invalid DASH manifest");
    return false;
}

void NetworkManager::setupUDPConnection(const QString &url)
{
    Logger::info("NetworkManager: Setting up UDP connection");
    
    QUrl qurl(url);
    QString host = qurl.host();
    quint16 port = qurl.port(1234); // Default port
    
    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onUdpDataReceived);
    
    if (m_udpSocket->bind(QHostAddress::Any, port)) {
        Logger::info("NetworkManager: UDP socket bound to port " + QString::number(port));
        
        // Join multicast group if applicable
        QHostAddress groupAddress(host);
        if (groupAddress.isMulticast()) {
            if (m_udpSocket->joinMulticastGroup(groupAddress)) {
                Logger::info("NetworkManager: Joined multicast group: " + host);
                setConnectionState(ConnectionState::Connected);
                startStatisticsTimer();
            } else {
                Logger::error("NetworkManager: Failed to join multicast group: " + host);
                setConnectionState(ConnectionState::Error);
            }
        } else {
            setConnectionState(ConnectionState::Connected);
            startStatisticsTimer();
        }
    } else {
        Logger::error("NetworkManager: Failed to bind UDP socket");
        setConnectionState(ConnectionState::Error);
    }
}

void NetworkManager::setupHTTPConnection(const QString &url)
{
    Logger::info("NetworkManager: Setting up HTTP connection");
    
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "TVTest-Android/1.0");
    request.setRawHeader("Accept", "*/*");
    
    m_currentReply = m_networkAccess->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onNetworkReplyFinished);
    connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &NetworkManager::onNetworkError);
}

void NetworkManager::startStatisticsTimer()
{
    if (!m_statisticsTimer->isActive()) {
        m_statisticsTimer->start(1000); // Update every second
    }
}

void NetworkManager::stopStatisticsTimer()
{
    if (m_statisticsTimer && m_statisticsTimer->isActive()) {
        m_statisticsTimer->stop();
    }
}

// NetworkStreamWorker implementation
NetworkStreamWorker::NetworkStreamWorker(QObject *parent)
    : QObject(parent)
    , m_isProcessing(false)
{
}

void NetworkStreamWorker::processStream(const QString &url)
{
    QMutexLocker locker(&m_processingMutex);
    
    if (m_isProcessing) {
        return;
    }
    
    m_isProcessing = true;
    
    if (url.startsWith("udp://")) {
        processUDPStream(url);
    } else {
        processHTTPStream(url);
    }
}

void NetworkStreamWorker::stopProcessing()
{
    QMutexLocker locker(&m_processingMutex);
    m_isProcessing = false;
}

void NetworkStreamWorker::processUDPStream(const QString &url)
{
    // TODO: Implement threaded UDP stream processing
    Q_UNUSED(url)
}

void NetworkStreamWorker::processHTTPStream(const QString &url)
{
    // TODO: Implement threaded HTTP stream processing
    Q_UNUSED(url)
}

// #include "NetworkManager.moc" // Removed - Qt6 auto-generates MOC files