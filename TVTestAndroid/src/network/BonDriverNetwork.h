#ifndef BONDRIVERNETWORK_H
#define BONDRIVERNETWORK_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QDataStream>
#include <QByteArray>
#include <QString>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

// 前方宣言
class BonDriverNetwork;

/**
 * @brief UIスレッドと独立した継続コマンド送信ワーカースレッド
 */
class ContinuousCommandWorker : public QObject
{
    Q_OBJECT
public:
    ContinuousCommandWorker(BonDriverNetwork* parent);
    void startWorker();
    void stopWorker();

public slots:
    void run();

private:
    BonDriverNetwork* m_bonDriver;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_running;
    bool m_stopRequested;
};

/**
 * @brief BonDriver_Proxyプロトコルでの通信を行うクラス
 * 
 * baruma.f5.si:1192への接続とBonDriver制御を実装
 */
class BonDriverNetwork : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief BonDriver_Proxyコマンド列挙
     */
    enum BonDriverCommand {
        eSelectBonDriver = 0,
        eCreateBonDriver = 1,
        eOpenTuner = 2,
        eCloseTuner = 3,
        eSetChannel = 4,
        eGetSignalLevel = 5,
        eWaitTsStream = 6,
        eGetReadyCount = 7,
        eGetTsStream = 8,
        ePurgeTsStream = 9,
        eRelease = 10,
        eGetTunerName = 11,
        eIsTunerOpening = 12,
        eEnumTuningSpace = 13,
        eEnumChannelName = 14,
        eSetChannel2 = 15,
    };

    /**
     * @brief チューニング空間定義
     */
    enum TuningSpace {
        TERRESTRIAL = 0,
        BS = 1,
        CS = 2
    };

    explicit BonDriverNetwork(QObject *parent = nullptr);
    ~BonDriverNetwork();

    bool connectToServer(const QString &host = "baruma.f5.si", int port = 1192);
    void disconnectFromServer();
    bool selectBonDriver(const QString &bonDriver);
    bool setChannel(TuningSpace space, uint32_t channel);
    void startReceiving();
    void stopReceiving();
    bool isConnected() const;
    float getSignalLevel() const;

    // ソケット取得
    QTcpSocket* socket() const { return m_socket; }
    
    // スレッドセーフなコマンド送信（ワーカースレッド用）
    bool sendCommandThreadSafe(BonDriverCommand command, const QByteArray &data = QByteArray());
    
    // ワーカースレッドからのアクセス用
    bool isTsStreamActiveForWorker() const { return m_isTsStreamActive; }

signals:
    void connected();
    void disconnected();
    void tsDataReceived(const QByteArray &data);
    void channelChanged(TuningSpace space, uint32_t channel);
    void signalLevelChanged(float level);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onHeartbeatTimeout(); // プッシュ型データ受信監視用
    // 【削除】onContinuousCommand - ワーカースレッドに移行
    // 【削除】プル型メソッド - プッシュ型実装では不要
    // void onTsReceiveTimer();
    // void continuousReceive();

private:
    bool sendCommand(BonDriverCommand command, const QByteArray &data = QByteArray());
    void processResponse();
    QString getCommandName(BonDriverCommand command) const;
    bool processCommandResponse();

private:
    QTcpSocket  *m_socket;
    QTimer      *m_heartbeatTimer;       // プッシュ型データ受信監視タイマー
    // 【変更】QTimer → ワーカースレッドに移行
    QThread     *m_workerThread;         // ワーカースレッド
    ContinuousCommandWorker *m_worker;   // 継続コマンド送信ワーカー
    // 【削除】QTimer *m_tsReceiveTimer; - プッシュ型実装では不要
    QByteArray   m_receiveBuffer;
    bool         m_isTsStreamActive;

    QString      m_currentBonDriver;
    TuningSpace  m_currentSpace;
    uint32_t     m_currentChannel;
    float        m_signalLevel;

    bool         m_isInitialized;
    bool         m_isTunerOpen;
};

#endif // BONDRIVERNETWORK_H
