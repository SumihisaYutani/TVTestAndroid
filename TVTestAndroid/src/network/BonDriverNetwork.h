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
        eSelectBonDriver = 0,     // BonDriver選択
        eCreateBonDriver = 1,     // インスタンス作成
        eOpenTuner = 2,          // チューナー開放
        eCloseTuner = 3,         // チューナー閉鎖
        eSetChannel = 4,         // チャンネル設定
        eGetSignalLevel = 5,     // 信号レベル取得
        eWaitTsStream = 6,       // TSストリーム待機
        eGetReadyCount = 7,      // 準備カウント取得
        eGetTsStream = 8,        // TSストリーム取得
        ePurgeTsStream = 9,      // TSストリームパージ
        eRelease = 10,           // リソース解放
        eGetTunerName = 11,      // チューナー名取得
        eIsTunerOpening = 12,    // チューナーオープン状態確認
        eEnumTuningSpace = 13,   // チューニング空間列挙
        eEnumChannelName = 14,   // チャンネル名列挙
        eSetChannel2 = 15,       // 拡張チャンネル設定（推奨）
    };

    /**
     * @brief チューニング空間定義
     */
    enum TuningSpace {
        TERRESTRIAL = 0,   // 地上波
        BS = 1,           // BS
        CS = 2            // CS
    };

    /**
     * @brief コンストラクタ
     */
    explicit BonDriverNetwork(QObject *parent = nullptr);
    
    /**
     * @brief デストラクタ
     */
    ~BonDriverNetwork();

    /**
     * @brief サーバーへの接続
     * @param host ホスト名（デフォルト: baruma.f5.si）
     * @param port ポート番号（デフォルト: 1192）
     * @return 接続成功時true
     */
    bool connectToServer(const QString &host = "baruma.f5.si", int port = 1192);
    
    /**
     * @brief サーバーからの切断
     */
    void disconnectFromServer();
    
    /**
     * @brief BonDriver選択
     * @param bonDriver BonDriver名（"PX-T" or "PT-S"）
     * @return 成功時true
     */
    bool selectBonDriver(const QString &bonDriver);
    
    /**
     * @brief チャンネル設定
     * @param space チューニング空間
     * @param channel チャンネル番号
     * @return 成功時true
     */
    bool setChannel(TuningSpace space, uint32_t channel);
    
    /**
     * @brief TSストリーム受信開始
     */
    void startReceiving();
    
    /**
     * @brief TSストリーム受信停止
     */
    void stopReceiving();
    
    /**
     * @brief 接続状態取得
     * @return 接続中の場合true
     */
    bool isConnected() const;
    
    /**
     * @brief 信号レベル取得
     * @return 信号レベル（0.0-1.0）
     */
    float getSignalLevel() const;

signals:
    /**
     * @brief 接続完了シグナル
     */
    void connected();
    
    /**
     * @brief 切断シグナル
     */
    void disconnected();
    
    /**
     * @brief TSデータ受信シグナル
     * @param data TSストリームデータ
     */
    void tsDataReceived(const QByteArray &data);
    
    /**
     * @brief チャンネル変更完了シグナル
     * @param space チューニング空間
     * @param channel チャンネル番号
     */
    void channelChanged(TuningSpace space, uint32_t channel);
    
    /**
     * @brief 信号レベル変更シグナル
     * @param level 信号レベル（0.0-1.0）
     */
    void signalLevelChanged(float level);
    
    /**
     * @brief エラーシグナル
     * @param error エラーメッセージ
     */
    void errorOccurred(const QString &error);

private slots:
    /**
     * @brief TCP接続完了ハンドラ
     */
    void onConnected();
    
    /**
     * @brief TCP切断ハンドラ
     */
    void onDisconnected();
    
    /**
     * @brief データ受信ハンドラ
     */
    void onReadyRead();
    
    /**
     * @brief TCP接続エラーハンドラ
     */
    void onSocketError(QAbstractSocket::SocketError error);
    
    /**
     * @brief TSストリーム取得タイマーハンドラ
     */
    void onTsReceiveTimer();

private:
    /**
     * @brief コマンド送信
     * @param command コマンド番号
     * @param data データペイロード
     * @return 送信成功時true
     */
    bool sendCommand(BonDriverCommand command, const QByteArray &data = QByteArray());
    
    /**
     * @brief レスポンス処理
     */
    void processResponse();
    
    /**
     * @brief コマンド名取得
     * @param command コマンド番号
     * @return コマンド名文字列
     */
    QString getCommandName(BonDriverCommand command) const;

    /**
     * @brief コマンドレスポンス処理
     * @return 処理成功時true
     */
    bool processCommandResponse();

private:
    QTcpSocket *m_socket;           // TCP接続
    QTimer *m_tsReceiveTimer;       // TSストリーム受信タイマー
    QByteArray m_receiveBuffer;     // 受信バッファ
    bool m_isTsStreamActive;        // TSストリーム受信中フラグ
    
    QString m_currentBonDriver;     // 現在選択中のBonDriver
    TuningSpace m_currentSpace;     // 現在のチューニング空間
    uint32_t m_currentChannel;      // 現在のチャンネル
    float m_signalLevel;            // 現在の信号レベル
    
    bool m_isInitialized;           // 初期化状態
    bool m_isTunerOpen;            // チューナー開放状態
};

#endif // BONDRIVERNETWORK_H