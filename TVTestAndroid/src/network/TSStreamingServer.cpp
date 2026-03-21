#include "TSStreamingServer.h"
#include "utils/Logger.h"
#include <QDateTime>
#include <QUrl>

TSStreamingServer::TSStreamingServer(QObject *parent)
    : QTcpServer(parent)
    , m_streamingTimer(new QTimer(this))
    , m_port(8080)
    , m_connectedClients(0)
{
    // ストリーミング配信タイマー設定
    m_streamingTimer->setInterval(STREAMING_INTERVAL);
    connect(m_streamingTimer, &QTimer::timeout, this, &TSStreamingServer::onStreamingTimer);
    
    LOG_INFO("TSStreamingServer初期化完了");
}

TSStreamingServer::~TSStreamingServer()
{
    stopServer();
}

bool TSStreamingServer::startServer(quint16 port)
{
    m_port = port;
    
    if (!listen(QHostAddress::LocalHost, port)) {
        LOG_CRITICAL(QString("HTTPサーバー起動失敗: %1").arg(errorString()));
        return false;
    }
    
    // ストリーミングタイマー開始
    m_streamingTimer->start();
    
    LOG_INFO(QString("TSストリーミングサーバー開始: http://127.0.0.1:%1/stream.ts").arg(port));
    return true;
}

void TSStreamingServer::stopServer()
{
    // 全クライアント切断
    for (auto* client : m_clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    m_clients.clear();
    
    // サーバー停止
    close();
    m_streamingTimer->stop();
    
    // キュークリア
    QMutexLocker locker(&m_dataMutex);
    m_tsDataQueue.clear();
    
    m_connectedClients = 0;
    LOG_INFO("TSストリーミングサーバー停止");
}

QString TSStreamingServer::getStreamUrl() const
{
    return QString("http://127.0.0.1:%1/stream.ts").arg(m_port);
}

void TSStreamingServer::addTSData(const QByteArray &data)
{
    // クライアントが接続中なら即座に送信（タイマー待ちなし）
    if (!m_clients.isEmpty()) {
        sendTSData(data);
        return;
    }

    // クライアント未接続時はキューに積む（接続待ち）
    QMutexLocker locker(&m_dataMutex);
    while (m_tsDataQueue.size() >= MAX_QUEUE_SIZE) {
        m_tsDataQueue.dequeue();
    }
    m_tsDataQueue.enqueue(data);
}

void TSStreamingServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *client = new QTcpSocket(this);
    if (!client->setSocketDescriptor(socketDescriptor)) {
        delete client;
        return;
    }
    
    connect(client, &QTcpSocket::readyRead, this, [this, client]() {
        QByteArray request = client->readAll();
        handleHttpRequest(client, request);
    });
    
    connect(client, &QTcpSocket::disconnected, this, &TSStreamingServer::onClientDisconnected);
    
    m_clients.append(client);
    m_connectedClients++;
    
    LOG_INFO(QString("VLCクライアント接続: %1 (合計: %2)")
             .arg(client->peerAddress().toString())
             .arg(m_connectedClients));
}

void TSStreamingServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    m_clients.removeAll(client);
    m_connectedClients--;
    client->deleteLater();
    
    LOG_INFO(QString("VLCクライアント切断 (残り: %1)").arg(m_connectedClients));
}

void TSStreamingServer::handleHttpRequest(QTcpSocket *socket, const QByteArray &request)
{
    QString requestStr = QString::fromUtf8(request);

    // HTTP GET /stream.ts を確認
    if (requestStr.startsWith("GET /stream.ts") || requestStr.startsWith("GET /")) {
        LOG_INFO("VLCからストリーミング要求受信 - キューをリセットして新規配信開始");

        // 古いTSデータを破棄（PTS不整合防止）
        {
            QMutexLocker locker(&m_dataMutex);
            m_tsDataQueue.clear();
        }

        sendHttpHeaders(socket);
    } else {
        // 404エラー応答
        QString response = "HTTP/1.1 404 Not Found\r\n\r\n";
        socket->write(response.toUtf8());
        socket->disconnectFromHost();
    }
}

void TSStreamingServer::sendHttpHeaders(QTcpSocket *socket)
{
    QString headers = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: video/mp2t\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n";
    
    socket->write(headers.toUtf8());
    LOG_INFO("HTTPヘッダー送信完了");
}

void TSStreamingServer::onStreamingTimer()
{
    static int timerCount = 0;
    timerCount++;
    
    if (timerCount <= 10 || timerCount % 100 == 0) {
        LOG_INFO(QString("⏰ タイマー配信処理 #%1 - クライアント数: %2, キューサイズ: %3")
                 .arg(timerCount).arg(m_clients.size()).arg(m_tsDataQueue.size()));
    }
    
    if (m_clients.isEmpty()) {
        if (timerCount <= 10) {
            LOG_WARNING("⚠️ VLCクライアントが未接続 - 配信スキップ");
        }
        return; // クライアントなし
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // キューからTSデータを取得して配信
    int sendDataCount = 0;
    
    while (!m_tsDataQueue.isEmpty() && !m_clients.isEmpty()) {
        QByteArray tsData = m_tsDataQueue.dequeue();
        locker.unlock();
        
        sendTSData(tsData);
        sendDataCount++;
        
        locker.relock();
    }
    
    if (sendDataCount > 0 && (timerCount <= 10 || timerCount % 100 == 0)) {
        LOG_INFO(QString("✅ タイマー配信完了: %1個のTSデータを送信").arg(sendDataCount));
    }
}

void TSStreamingServer::sendTSData(const QByteArray &data)
{
    // 切断されたクライアントを削除
    auto it = m_clients.begin();
    while (it != m_clients.end()) {
        QTcpSocket *client = *it;
        
        if (client->state() != QAbstractSocket::ConnectedState) {
            it = m_clients.erase(it);
            m_connectedClients--;
            client->deleteLater();
            continue;
        }
        
        // HTTP Chunked形式でTSデータを送信
        QString chunkHeader = QString("%1\r\n").arg(data.size(), 0, 16);
        client->write(chunkHeader.toUtf8());
        client->write(data);
        client->write("\r\n");
        
        static int sendCount = 0;
        sendCount++;
        if (sendCount <= 10 || sendCount % 100 == 0) {
            LOG_INFO(QString("🚀 VLCにTSデータ送信 #%1 - %2 bytes").arg(sendCount).arg(data.size()));
        }
        
        ++it;
    }
}

