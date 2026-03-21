#pragma once
#include <QMainWindow>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QComboBox>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include "network/BonDriverNetwork.h"
#include "network/VLCStreamingPlayer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSelectBonDriverClicked();
    void onSetChannelClicked();
    void onStartReceivingClicked();
    void onStopReceivingClicked();
    void onClearLogClicked();
    void onQuickChannelSelected();

    // VLCプレイヤーイベント
    void onStreamingStarted();
    void onStreamingStopped();
    void onPlayerStateChanged(VLCStreamingPlayer::PlayerState state);
    void onSignalLevelChanged(float level);
    void onPlayerErrorOccurred(const QString &error);
    void onBufferingProgress(float percentage);
    void onUpdateStatsTimer();

private:
    void setupUI();
    void setupConnections();
    void setupQuickChannelSelection();
    void addLogMessage(const QString &message);
    void updateUIState();
    void saveSettings();
    void restoreSettings();

    // VLCストリーミングプレイヤー
    VLCStreamingPlayer               *m_vlcPlayer;
    QWidget                          *m_videoWidget;  // VLC表示用ウィジェット

    // UI
    QWidget     *m_centralWidget;
    QVBoxLayout *m_mainLayout;

    QGroupBox   *m_connectionGroup;
    QGridLayout *m_connectionLayout;
    QComboBox   *m_serverCombo;
    QSpinBox    *m_portSpin;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QLabel      *m_connectionStatus;

    QGroupBox   *m_bonDriverGroup;
    QGridLayout *m_bonDriverLayout;
    QComboBox   *m_bonDriverCombo;
    QPushButton *m_selectBonDriverButton;
    QLabel      *m_bonDriverStatus;

    QGroupBox   *m_channelGroup;
    QGridLayout *m_channelLayout;
    QComboBox   *m_spaceCombo;
    QSpinBox    *m_channelSpin;
    QPushButton *m_setChannelButton;
    QLabel      *m_channelStatus;

    QGroupBox    *m_streamGroup;
    QGridLayout  *m_streamLayout;
    QPushButton  *m_startReceivingButton;
    QPushButton  *m_stopReceivingButton;
    QLabel       *m_streamStatus;
    QProgressBar *m_signalLevelBar;

    QGroupBox   *m_quickChannelGroup;
    QHBoxLayout *m_quickChannelLayout;
    QComboBox   *m_quickChannelCombo;
    QPushButton *m_quickChannelButton;

    QGroupBox   *m_statsGroup;
    QGridLayout *m_statsLayout;
    QLabel      *m_totalBytesLabel;
    QLabel      *m_packetsLabel;
    QLabel      *m_bitrateLabel;

    QTextEdit   *m_logTextEdit;
    QPushButton *m_clearLogButton;

    QTimer    *m_updateStatsTimer;
    qint64     m_totalBytes      = 0;
    qint64     m_totalPackets    = 0;
    QDateTime  m_startTime;
    QDateTime  m_lastUpdateTime;
    qint64     m_lastTotalBytes  = 0;

    // ログバッファ（processEvents不要化）
    QStringList m_pendingLogs;
    QTimer     *m_logFlushTimer;
};
