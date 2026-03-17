#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QProgressBar>
#include <QGroupBox>
#include <QTimer>
#include <QDateTime>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QBuffer>
#include <QSettings>

#include "network/BonDriverNetwork.h"

// Forward declarations
#ifdef USE_FFMPEG
class FFmpegDecoder;
class FFmpegVideoWidget;
#endif
class DirectStreamPlayer;

/**
 * @brief BonDriver_Proxy接続テスト用メインウィンドウ
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * @brief 接続ボタンクリック
     */
    void onConnectClicked();
    
    /**
     * @brief 切断ボタンクリック
     */
    void onDisconnectClicked();
    
    /**
     * @brief BonDriver選択ボタンクリック
     */
    void onSelectBonDriverClicked();
    
    /**
     * @brief チャンネル設定ボタンクリック
     */
    void onSetChannelClicked();
    
    /**
     * @brief TSストリーム受信開始ボタンクリック
     */
    void onStartReceivingClicked();
    
    /**
     * @brief TSストリーム受信停止ボタンクリック
     */
    void onStopReceivingClicked();
    
    /**
     * @brief 直接ストリーミング開始ボタンクリック
     */
    void onStartDirectStreamClicked();
    
    /**
     * @brief 直接ストリーミング停止ボタンクリック
     */
    void onStopDirectStreamClicked();
    
    /**
     * @brief ログクリアボタンクリック
     */
    void onClearLogClicked();
    
    /**
     * @brief BonDriverNetwork接続完了
     */
    void onNetworkConnected();
    
    /**
     * @brief BonDriverNetwork切断
     */
    void onNetworkDisconnected();
    
    /**
     * @brief TSデータ受信
     */
    void onTsDataReceived(const QByteArray &data);
    
    /**
     * @brief チャンネル変更完了
     */
    void onChannelChanged(BonDriverNetwork::TuningSpace space, uint32_t channel);
    
    /**
     * @brief 信号レベル変更
     */
    void onSignalLevelChanged(float level);
    
    /**
     * @brief エラー発生
     */
    void onErrorOccurred(const QString &error);
    
    /**
     * @brief 統計更新タイマー
     */
    void onUpdateStatsTimer();
    
    /**
     * @brief クイックチャンネル選択
     */
    void onQuickChannelSelected();
    
    // DirectStreamPlayer関連スロット
    /**
     * @brief 直接ストリーミング再生状態変更
     */
    void onDirectStreamPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    
    /**
     * @brief 直接ストリーミングメディア情報変更
     */
    void onDirectStreamMediaInfoChanged(const QString &info);
    
    /**
     * @brief 直接ストリーミングエラー
     */
    void onDirectStreamErrorOccurred(const QString &error);
    
    /**
     * @brief 直接ストリーミングバッファ状態変更
     */
    void onDirectStreamBufferStatusChanged(qint64 bufferSize, int bufferStatus);

    /**
     * @brief 設定の保存
     */
    void saveSettings();
    
    /**
     * @brief 設定の復元
     */
    void restoreSettings();

private:
    /**
     * @brief UI初期化
     */
    void setupUI();
    
    /**
     * @brief シグナル接続
     */
    void setupConnections();
    
    /**
     * @brief メディアプレイヤー設定
     */
    void setupMediaPlayer();
    
    /**
     * @brief 直接ストリーミングプレイヤー設定
     */
    void setupDirectStreamPlayer();
    
    /**
     * @brief クイックチャンネル選択設定（TVTest .ch2ファイルベース）
     */
    void setupQuickChannelSelection();
    
    /**
     * @brief ログメッセージ追加
     */
    void addLogMessage(const QString &message);
    
    /**
     * @brief UI状態更新
     */
    void updateUIState();
    
    /**
     * @brief 外部プレイヤー起動
     */
    bool launchExternalPlayer(const QString &tsFilePath);
    
    /**
     * @brief ライブストリーム更新開始
     */
    void startLiveStreamUpdate(const QString &tsFilePath);

private:
    // ネットワーク
    BonDriverNetwork *m_network;
    
    // UI コンポーネント
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    
    // 接続グループ
    QGroupBox *m_connectionGroup;
    QGridLayout *m_connectionLayout;
    QLineEdit *m_serverEdit;
    QSpinBox *m_portSpin;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QLabel *m_connectionStatus;
    
    // BonDriverグループ
    QGroupBox *m_bonDriverGroup;
    QGridLayout *m_bonDriverLayout;
    QComboBox *m_bonDriverCombo;
    QPushButton *m_selectBonDriverButton;
    QLabel *m_bonDriverStatus;
    
    // チャンネルグループ
    QGroupBox *m_channelGroup;
    QGridLayout *m_channelLayout;
    QComboBox *m_spaceCombo;
    QSpinBox *m_channelSpin;
    QPushButton *m_setChannelButton;
    QLabel *m_channelStatus;
    
    // TSストリームグループ
    QGroupBox *m_streamGroup;
    QGridLayout *m_streamLayout;
    QPushButton *m_startReceivingButton;
    QPushButton *m_stopReceivingButton;
    QLabel *m_streamStatus;
    QProgressBar *m_signalLevelBar;
    
    // 統計グループ
    QGroupBox *m_statsGroup;
    QGridLayout *m_statsLayout;
    QLabel *m_totalBytesLabel;
    QLabel *m_packetsLabel;
    QLabel *m_bitrateLabel;
    
    // ログ
    QTextEdit *m_logTextEdit;
    QPushButton *m_clearLogButton;
    
    // 統計
    QTimer *m_updateStatsTimer;
    qint64 m_totalBytes;
    qint64 m_totalPackets;
    QDateTime m_startTime;
    QDateTime m_lastUpdateTime;
    qint64 m_lastTotalBytes;
    
    // チャンネル選択（TVTest .ch2ファイルベース）
    QGroupBox *m_quickChannelGroup;
    QHBoxLayout *m_quickChannelLayout;
    QComboBox *m_quickChannelCombo;
    QPushButton *m_quickChannelButton;
    
    // 動画再生 (Qt Multimedia) - 将来的に削除予定
    QVideoWidget *m_videoWidget;
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QBuffer *m_tsBuffer;
    QByteArray m_tsData;
    
    // FFmpeg TSストリーム再生
#ifdef USE_FFMPEG
    FFmpegDecoder *m_ffmpegDecoder;
    FFmpegVideoWidget *m_ffmpegVideoWidget;
#endif
    
    // 直接ストリーミング再生（元TVTest同様）
    DirectStreamPlayer *m_directStreamPlayer;
    QVideoWidget *m_directVideoWidget;
    
    // 直接ストリーミング制御UI
    QGroupBox *m_directStreamGroup;
    QHBoxLayout *m_directStreamLayout;
    QPushButton *m_startDirectStreamButton;
    QPushButton *m_stopDirectStreamButton;
    QLabel *m_directStreamStatus;
    QLabel *m_bufferStatus;
};

#endif // MAINWINDOW_H