#ifndef TSSTREAMINGSERVER_H
#define TSSTREAMINGSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
#include <QMutex>
#include <QQueue>

/**
 * @brief TSデータをHTTPストリーミング配信するローカルサーバー
 * 
 * VLCが "http://127.0.0.1:8080/stream.ts" でアクセス可能
 * コールバック不要の真のストリーミング再生
 */
class TSStreamingServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit TSStreamingServer(QObject *parent = nullptr);
    ~TSStreamingServer();

    bool startServer(quint16 port = 8080);
    void stopServer();
    QString getStreamUrl() const;
    
    // TSデータを配信キューに追加
    void addTSData(const QByteArray &data);
    
    bool isClientConnected() const { return m_connectedClients > 0; }

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onClientDisconnected();
    void onStreamingTimer();

private:
    void handleHttpRequest(QTcpSocket *socket, const QByteArray &request);
    void sendHttpHeaders(QTcpSocket *socket);
    void sendTSData(const QByteArray &data);

private:
    QList<QTcpSocket*> m_clients;
    QQueue<QByteArray> m_tsDataQueue;
    QMutex m_dataMutex;
    QTimer *m_streamingTimer;
    quint16 m_port;
    int m_connectedClients;
    
    static const int MAX_QUEUE_SIZE = 1000; // 最大キューサイズ（元の値に戻す）
    static const int STREAMING_INTERVAL = 20; // 20ms間隔で配信（元の値）
};

#endif // TSSTREAMINGSERVER_H