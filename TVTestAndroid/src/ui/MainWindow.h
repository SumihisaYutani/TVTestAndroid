#pragma once
#include <QMainWindow>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QComboBox>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QQueue>
#include "network/BonDriverNetwork.h"
#include "network/HighPerformanceStreamProcessor.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSelectBonDriverClicked();
    void onSetChannelClicked();
    void onStartReceivingClicked();
    void onStopReceivingClicked();
    void onClearLogClicked();
    void onQuickChannelSelected();

    // ネットワークシグナル
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onTsDataReceived(const QByteArray &data);
    void onChannelChanged(BonDriverNetwork::TuningSpace space, uint32_t channel);
    void onSignalLevelChanged(float level);
    void onErrorOccurred(const QString &error);
    
    // 高性能ストリーミングシグナル
    void onRealTimeStreamingStarted();
    void onStreamingStatsUpdated(const HighPerformanceStreamProcessor::StreamingStats& stats);
    void onStreamingError(const QString& error);
    void onUpdateStatsTimer();

private:
    void setupUI();
    void setupConnections();
    void setupMediaPlayer();
    void setupQuickChannelSelection();
    void addLogMessage(const QString &message);
    void updateUIState();
    void saveSettings();
    void restoreSettings();
    
    // FFplay連携
    void saveTsDataAndStartFFplay(const QByteArray &data);
    void startFFplay();
    void stopFFplay();
    void flushTsBuffer();  // バッファフラッシュ
    
    // ストリーミング再生
    void setupStreamingPipe();
    void writeToStreamPipe(const QByteArray &data);
    void startStreamingPlayback();
    bool validateTsPacket(const QByteArray &data);
    
    // レートコントロール
    void setupRateControl();
    void sendStreamData();
    
    // PCRベース同期
    qint64 extractPCR(const QByteArray &tsPacket);
    void sendPacketWithPCRTiming(const QByteArray &packet, qint64 pcr);

    // ネットワーク
    BonDriverNetwork *m_network;
    
    // 【新実装】ゼロコピー・高性能ストリーミングプロセッサ
    HighPerformanceStreamProcessor *m_streamProcessor;

    // 動画表示
    QVideoWidget     *m_videoWidget;

    // Qt Multimedia
    QMediaPlayer     *m_mediaPlayer;
    QAudioOutput     *m_audioOutput;

    // UI
    QWidget     *m_centralWidget;
    QVBoxLayout *m_mainLayout;

    // 接続グループ
    QGroupBox   *m_connectionGroup;
    QGridLayout *m_connectionLayout;
    QLineEdit   *m_serverEdit;
    QSpinBox    *m_portSpin;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QLabel      *m_connectionStatus;

    // BonDriverグループ
    QGroupBox   *m_bonDriverGroup;
    QGridLayout *m_bonDriverLayout;
    QComboBox   *m_bonDriverCombo;
    QPushButton *m_selectBonDriverButton;
    QLabel      *m_bonDriverStatus;

    // チャンネルグループ
    QGroupBox   *m_channelGroup;
    QGridLayout *m_channelLayout;
    QComboBox   *m_spaceCombo;
    QSpinBox    *m_channelSpin;
    QPushButton *m_setChannelButton;
    QLabel      *m_channelStatus;

    // TSストリームグループ
    QGroupBox    *m_streamGroup;
    QGridLayout  *m_streamLayout;
    QPushButton  *m_startReceivingButton;
    QPushButton  *m_stopReceivingButton;
    QLabel       *m_streamStatus;
    QProgressBar *m_signalLevelBar;

    // クイックチャンネル
    QGroupBox   *m_quickChannelGroup;
    QHBoxLayout *m_quickChannelLayout;
    QComboBox   *m_quickChannelCombo;
    QPushButton *m_quickChannelButton;

    // 統計グループ
    QGroupBox   *m_statsGroup;
    QGridLayout *m_statsLayout;
    QLabel      *m_totalBytesLabel;
    QLabel      *m_packetsLabel;
    QLabel      *m_bitrateLabel;

    // ログ
    QTextEdit   *m_logTextEdit;
    QPushButton *m_clearLogButton;

    // 統計
    QTimer    *m_updateStatsTimer;
    qint64     m_totalBytes   = 0;
    qint64     m_totalPackets = 0;
    QDateTime  m_startTime;
    QDateTime  m_lastUpdateTime;
    qint64     m_lastTotalBytes = 0;
    
    // FFplay連携
    QProcess  *m_ffplayProcess = nullptr;
    QString    m_tsFilePath;
    bool       m_ffplayStarted = false;
    QByteArray m_tsBuffer;           // TSデータバッファ
    QTimer    *m_flushTimer;         // 定期的なフラッシュ用タイマー
    
    // ストリーミング再生
    QString    m_pipeName;           // Named Pipe名
    QProcess  *m_streamProcess = nullptr;
    bool       m_streamingMode = false; // ストリーミングモード無効（ファイル保存方式）
    
    // レートコントロール
    QTimer    *m_streamingTimer;     // ストリーミング送信タイマー
    QByteArray m_streamingBuffer;    // レート制御用バッファ
    qint64     m_lastSendTime = 0;   // 最終送信時刻
    double     m_targetBitrate = 0;  // 目標ビットレート
    
    // PCR同期
    qint64     m_firstPCR = -1;      // 最初のPCR値
    qint64     m_streamStartTime = 0; // ストリーミング開始時刻
    QQueue<QPair<QByteArray, qint64>> m_pcrPacketQueue; // PCR付きパケットキュー
};
