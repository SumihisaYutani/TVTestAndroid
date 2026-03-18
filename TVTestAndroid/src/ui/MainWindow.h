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
#include "BonDriverNetwork.h"
#include "media/LibVLCPlayer.h"

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

    // ネットワーク
    BonDriverNetwork *m_network;

    // libVLC再生
    LibVLCPlayer     *m_vlcPlayer;
    QWidget          *m_videoWidget;   // 映像描画先

    // Qt Multimedia（音声出力のみ残す場合は削除可）
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
};
