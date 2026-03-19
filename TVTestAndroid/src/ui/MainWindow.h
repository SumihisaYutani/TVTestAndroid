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
#include "BonDriverNetwork.h"
#include "media/FfmpegPipePlayer.h"

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
    void setupQuickChannelSelection();
    void addLogMessage(const QString &message);
    void updateUIState();
    void saveSettings();
    void restoreSettings();

    // ネットワーク
    BonDriverNetwork                  *m_network;

    // 再生
    FfmpegPipePlayer  *m_player;
    QWidget           *m_videoWidget;  // ffplay埋め込み先

    // UI
    QWidget     *m_centralWidget;
    QVBoxLayout *m_mainLayout;

    QGroupBox   *m_connectionGroup;
    QGridLayout *m_connectionLayout;
    QLineEdit   *m_serverEdit;
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
