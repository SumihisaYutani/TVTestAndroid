#ifndef BONDRIVERNETWORK_H
#define BONDRIVERNETWORK_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QDataStream>
#include <QByteArray>
#include <QString>
#include <QDebug>

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
    void onTsReceiveTimer();
    void continuousReceive();

private:
    bool sendCommand(BonDriverCommand command, const QByteArray &data = QByteArray());
    void processResponse();
    QString getCommandName(BonDriverCommand command) const;
    bool processCommandResponse();

private:
    QTcpSocket  *m_socket;
    QTimer      *m_tsReceiveTimer;
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
