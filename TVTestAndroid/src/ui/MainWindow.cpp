#include "MainWindow.h"
#include "utils/Logger.h"
#include <QDateTime>
#include <QApplication>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_vlcPlayer(new VLCStreamingPlayer(this))
    , m_videoWidget(new QWidget(this))
    , m_updateStatsTimer(new QTimer(this))
    , m_logFlushTimer(new QTimer(this))
{
    setupUI();
    setupConnections();

    // VLCプレイヤー初期化
    m_vlcPlayer->initializeVLC();
    m_vlcPlayer->setVideoWidget(m_videoWidget);


    updateUIState();
    restoreSettings();

    // 統計タイマー（1秒）
    m_updateStatsTimer->setInterval(1000);
    connect(m_updateStatsTimer, &QTimer::timeout,
            this, &MainWindow::onUpdateStatsTimer);

    // ログフラッシュタイマー（200ms・processEvents不要化）
    m_logFlushTimer->setInterval(200);
    connect(m_logFlushTimer, &QTimer::timeout, this, [this]() {
        if (m_pendingLogs.isEmpty()) return;
        for (const QString &line : m_pendingLogs)
            m_logTextEdit->append(line);
        m_pendingLogs.clear();
        QTextCursor cursor = m_logTextEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_logTextEdit->setTextCursor(cursor);
    });
    m_logFlushTimer->start();

    setWindowTitle("BonDriver Network Player");
    resize(1400, 1000);
    setMinimumSize(1200, 900);

    addLogMessage("アプリケーション開始");
}

MainWindow::~MainWindow()
{
    saveSettings();
    m_vlcPlayer->stopStreaming();
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QVBoxLayout(m_centralWidget);

    // 映像表示（ffplay埋め込み先）
m_videoWidget->setMinimumSize(640, 360);
m_videoWidget->setStyleSheet("background-color: black;");
m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_mainLayout->addWidget(m_videoWidget);

    // 接続グループ
    m_connectionGroup  = new QGroupBox("サーバー接続");
    m_connectionLayout = new QGridLayout(m_connectionGroup);
    m_connectionLayout->addWidget(new QLabel("サーバー:"), 0, 0);
    m_serverCombo = new QComboBox();
    m_serverCombo->setEditable(true);
    m_serverCombo->addItem("192.168.0.5");
    m_serverCombo->addItem("baruma.f5.si");
    m_connectionLayout->addWidget(m_serverCombo, 0, 1);
    m_connectionLayout->addWidget(new QLabel("ポート:"), 0, 2);
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(1192);
    m_connectionLayout->addWidget(m_portSpin, 0, 3);
    m_connectButton    = new QPushButton("接続");
    m_disconnectButton = new QPushButton("切断");
    m_connectionStatus = new QLabel("未接続");
    m_connectionLayout->addWidget(m_connectButton,    1, 0);
    m_connectionLayout->addWidget(m_disconnectButton, 1, 1);
    m_connectionLayout->addWidget(m_connectionStatus, 1, 2, 1, 2);
    m_mainLayout->addWidget(m_connectionGroup);

    // BonDriverグループ
    m_bonDriverGroup  = new QGroupBox("BonDriver選択");
    m_bonDriverLayout = new QGridLayout(m_bonDriverGroup);
    m_bonDriverCombo  = new QComboBox();
    m_bonDriverCombo->addItem("PT-T (地上波)");
    m_bonDriverCombo->addItem("PT-S (BS/CS)");
    m_bonDriverLayout->addWidget(m_bonDriverCombo, 0, 0);
    m_selectBonDriverButton = new QPushButton("BonDriver選択");
    m_bonDriverStatus       = new QLabel("未選択");
    m_bonDriverLayout->addWidget(m_selectBonDriverButton, 0, 1);
    m_bonDriverLayout->addWidget(m_bonDriverStatus,       0, 2);
    m_mainLayout->addWidget(m_bonDriverGroup);

    // チャンネルグループ
    m_channelGroup  = new QGroupBox("チャンネル設定");
    m_channelLayout = new QGridLayout(m_channelGroup);
    m_channelLayout->addWidget(new QLabel("チューニング空間:"), 0, 0);
    m_spaceCombo = new QComboBox();
    m_spaceCombo->addItem("地上波 (0)");
    m_spaceCombo->addItem("BS (1)");
    m_spaceCombo->addItem("CS (2)");
    m_channelLayout->addWidget(m_spaceCombo, 0, 1);
    m_channelLayout->addWidget(new QLabel("チャンネル:"), 0, 2);
    m_channelSpin = new QSpinBox();
    m_channelSpin->setRange(0, 999);
    m_channelSpin->setValue(14);
    m_channelLayout->addWidget(m_channelSpin, 0, 3);
    m_setChannelButton = new QPushButton("チャンネル設定");
    m_channelStatus    = new QLabel("未設定");
    m_channelLayout->addWidget(m_setChannelButton, 1, 0);
    m_channelLayout->addWidget(m_channelStatus,    1, 1, 1, 3);
    m_mainLayout->addWidget(m_channelGroup);

    // TSストリームグループ
    m_streamGroup  = new QGroupBox("TSストリーム");
    m_streamLayout = new QGridLayout(m_streamGroup);
    m_startReceivingButton = new QPushButton("受信開始");
    m_stopReceivingButton  = new QPushButton("受信停止");
    m_streamStatus         = new QLabel("停止中");
    m_streamLayout->addWidget(m_startReceivingButton, 0, 0);
    m_streamLayout->addWidget(m_stopReceivingButton,  0, 1);
    m_streamLayout->addWidget(m_streamStatus,         0, 2);
    m_streamLayout->addWidget(new QLabel("信号レベル:"), 1, 0);
    m_signalLevelBar = new QProgressBar();
    m_signalLevelBar->setRange(0, 100);
    m_streamLayout->addWidget(m_signalLevelBar, 1, 1, 1, 2);
    m_mainLayout->addWidget(m_streamGroup);

    // 統計グループ
    m_statsGroup  = new QGroupBox("受信統計");
    m_statsLayout = new QGridLayout(m_statsGroup);
    m_totalBytesLabel = new QLabel("総受信量: 0 MB");
    m_packetsLabel    = new QLabel("パケット数: 0");
    m_bitrateLabel    = new QLabel("ビットレート: 0 Mbps");
    m_statsLayout->addWidget(m_totalBytesLabel, 0, 0);
    m_statsLayout->addWidget(m_packetsLabel,    0, 1);
    m_statsLayout->addWidget(m_bitrateLabel,    0, 2);
    m_mainLayout->addWidget(m_statsGroup);

    // ログ
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setMaximumHeight(150);
    m_logTextEdit->setReadOnly(true);
    m_mainLayout->addWidget(m_logTextEdit);
    m_clearLogButton = new QPushButton("ログクリア");
    m_mainLayout->addWidget(m_clearLogButton);

    setupQuickChannelSelection();
}

void MainWindow::setupConnections()
{
    connect(m_connectButton,         &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton,      &QPushButton::clicked,
            this, &MainWindow::onDisconnectClicked);
    connect(m_selectBonDriverButton, &QPushButton::clicked,
            this, &MainWindow::onSelectBonDriverClicked);
    connect(m_setChannelButton,      &QPushButton::clicked,
            this, &MainWindow::onSetChannelClicked);
    connect(m_startReceivingButton,  &QPushButton::clicked,
            this, &MainWindow::onStartReceivingClicked);
    connect(m_stopReceivingButton,   &QPushButton::clicked,
            this, &MainWindow::onStopReceivingClicked);
    connect(m_clearLogButton,        &QPushButton::clicked,
            this, &MainWindow::onClearLogClicked);

    // VLCストリーミングプレイヤー
    connect(m_vlcPlayer, &VLCStreamingPlayer::streamingStarted,
            this, &MainWindow::onStreamingStarted);
    connect(m_vlcPlayer, &VLCStreamingPlayer::streamingStopped,
            this, &MainWindow::onStreamingStopped);
    connect(m_vlcPlayer, &VLCStreamingPlayer::stateChanged,
            this, &MainWindow::onPlayerStateChanged);
    connect(m_vlcPlayer, &VLCStreamingPlayer::signalLevelChanged,
            this, &MainWindow::onSignalLevelChanged);
    connect(m_vlcPlayer, &VLCStreamingPlayer::errorOccurred,
            this, &MainWindow::onPlayerErrorOccurred);
    connect(m_vlcPlayer, &VLCStreamingPlayer::bufferingProgress,
            this, &MainWindow::onBufferingProgress);

    // クイックチャンネル選択
    connect(m_quickChannelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onQuickChannelSelected);
}

void MainWindow::onConnectClicked()
{
    QString host = m_serverCombo->currentText();
    int port = m_portSpin->value();
    addLogMessage(QString("📡 BonDriverサーバー接続: %1:%2").arg(host).arg(port));
    
    // VLC初期化のみ（ストリーミングは受信開始ボタンで開始）
    if (!m_vlcPlayer->initializeVLC()) {
        addLogMessage("❌ VLC初期化失敗");
        return;
    }
    
    m_connectionStatus->setText("接続済み");
    addLogMessage("✅ 接続完了 - BonDriver・チャンネルを選択してから受信開始してください");
    updateUIState();
}

void MainWindow::onDisconnectClicked()
{
    m_vlcPlayer->stopStreaming();
    m_connectionStatus->setText("未接続");
    m_bonDriverStatus->setText("未選択");
    m_channelStatus->setText("未設定");
    addLogMessage("📡 切断完了");
    updateUIState();
}

void MainWindow::onSelectBonDriverClicked()
{
    QString bonDriver = (m_bonDriverCombo->currentIndex() == 0) ? "PT-T" : "PT-S";
    m_bonDriverStatus->setText(QString("選択済み: %1").arg(bonDriver));
    addLogMessage(QString("✅ BonDriver選択: %1").arg(bonDriver));
    
    updateUIState();
    saveSettings();
}

void MainWindow::onSetChannelClicked()
{
    auto space = static_cast<BonDriverNetwork::TuningSpace>(
                     m_spaceCombo->currentIndex());
    uint32_t channel = static_cast<uint32_t>(m_channelSpin->value());
    
    QString spaceName = (space == 0) ? "地上波" : (space == 1) ? "BS" : "CS";
    m_channelStatus->setText(QString("設定済み: %1 Ch%2").arg(spaceName).arg(channel));
    addLogMessage(QString("✅ チャンネル選択: %1 Ch%2").arg(spaceName).arg(channel));
    
    // 実際のチャンネル設定はストリーミング開始時に行う
    updateUIState();
    saveSettings();
}

void MainWindow::onStartReceivingClicked()
{
    QString host = m_serverCombo->currentText();
    int port = m_portSpin->value();
    
    addLogMessage("🚀 VLCストリーミング再生開始");
    
    // 実際のストリーミング開始
    if (m_vlcPlayer->startStreaming(host, port)) {
        addLogMessage("✅ VLCストリーミング開始成功");
        
        // 選択されたチャンネル設定を適用
        auto space = static_cast<BonDriverNetwork::TuningSpace>(m_spaceCombo->currentIndex());
        uint32_t channel = static_cast<uint32_t>(m_channelSpin->value());
        QString bonDriver = (m_bonDriverCombo->currentIndex() == 0) ? "PT-T" : "PT-S";
        
        addLogMessage(QString("📺 チャンネル設定適用: %1 %2 Ch%3")
                      .arg(bonDriver)
                      .arg((space == 0) ? "地上波" : (space == 1) ? "BS" : "CS")
                      .arg(channel));
        
        m_vlcPlayer->setChannel(space, channel);
        m_streamStatus->setText("🚀 VLC再生中");
        
        // 統計初期化
        m_totalBytes     = 0;
        m_totalPackets   = 0;
        m_startTime      = QDateTime::currentDateTime();
        m_lastUpdateTime = m_startTime;
        m_lastTotalBytes = 0;
        m_updateStatsTimer->start();
    } else {
        addLogMessage("❌ VLCストリーミング開始失敗");
        m_streamStatus->setText("開始失敗");
    }
    
    updateUIState();
}

void MainWindow::onStopReceivingClicked()
{
    addLogMessage("VLCストリーミング停止");
    m_vlcPlayer->stopStreaming();
    m_streamStatus->setText("停止中");
    m_updateStatsTimer->stop();
    updateUIState();
}

void MainWindow::onClearLogClicked()
{
    m_logTextEdit->clear();
    m_pendingLogs.clear();
}

// VLCプレイヤーイベントハンドラー
void MainWindow::onStreamingStarted()
{
    m_connectionStatus->setText("VLCストリーミング中");
    addLogMessage("✅ VLCストリーミング開始完了");
    updateUIState();
}

void MainWindow::onStreamingStopped()
{
    m_connectionStatus->setText("停止");
    addLogMessage("⏹ VLCストリーミング停止");
    updateUIState();
}

void MainWindow::onPlayerStateChanged(VLCStreamingPlayer::PlayerState state)
{
    const char* stateNames[] = {"停止", "再生中", "一時停止", "バッファリング", "エラー"};
    m_streamStatus->setText(QString("VLC: %1").arg(stateNames[state]));
    
    if (state == VLCStreamingPlayer::Playing) {
        addLogMessage("▶ VLC再生開始");
        m_startTime = QDateTime::currentDateTime();
        m_lastUpdateTime = m_startTime;
        m_updateStatsTimer->start();
    } else if (state == VLCStreamingPlayer::Stopped) {
        m_updateStatsTimer->stop();
    }
    
    updateUIState();
}

void MainWindow::onSignalLevelChanged(float level)
{
    m_signalLevelBar->setValue(static_cast<int>(level * 100));
}

void MainWindow::onPlayerErrorOccurred(const QString &error)
{
    addLogMessage(QString("❌ VLCエラー: %1").arg(error));
    updateUIState();
}

void MainWindow::onBufferingProgress(float percentage)
{
    addLogMessage(QString("📡 バッファリング: %1%").arg(percentage, 0, 'f', 1));
}

void MainWindow::onUpdateStatsTimer()
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsedMs = m_lastUpdateTime.msecsTo(now);

    m_totalBytesLabel->setText(
        QString("総受信量: %1 MB")
        .arg(m_totalBytes / (1024.0 * 1024.0), 0, 'f', 2));
    m_packetsLabel->setText(
        QString("パケット数: %1").arg(m_totalPackets));

    if (elapsedMs > 0) {
        qint64 delta = m_totalBytes - m_lastTotalBytes;
        double bitrate = (delta * 8.0 * 1000.0) / elapsedMs / (1024.0 * 1024.0);
        m_bitrateLabel->setText(
            QString("ビットレート: %1 Mbps").arg(bitrate, 0, 'f', 2));
    }

    m_lastUpdateTime = now;
    m_lastTotalBytes = m_totalBytes;
}

void MainWindow::addLogMessage(const QString &message)
{
    QString line = QString("[%1] %2")
                   .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                   .arg(message);
    // バッファに積んでタイマーで一括フラッシュ（processEvents不要）
    m_pendingLogs.append(line);
    LOG_INFO(message);
}

void MainWindow::updateUIState()
{
    bool streaming = m_vlcPlayer->isStreaming();
    bool connected = (m_connectionStatus->text() == "接続済み");
    VLCStreamingPlayer::PlayerState state = m_vlcPlayer->getState();
    
    // 新しい操作フロー対応
    m_connectButton->setEnabled(!connected && !streaming);
    m_disconnectButton->setEnabled(connected || streaming);
    m_selectBonDriverButton->setEnabled(connected);  // 接続後に選択可能
    m_bonDriverCombo->setEnabled(true);
    m_setChannelButton->setEnabled(connected);       // 接続後に設定可能
    m_startReceivingButton->setEnabled(connected && !streaming);  // 接続済み・未ストリーミング時
    m_stopReceivingButton->setEnabled(streaming);
    if (m_quickChannelGroup)
        m_quickChannelGroup->setEnabled(connected);  // 接続後に選択可能
}

void MainWindow::setupQuickChannelSelection()
{
    m_quickChannelGroup  = new QGroupBox("クイックチャンネル選択");
    m_quickChannelLayout = new QHBoxLayout(m_quickChannelGroup);

    struct Ch { QString name; int space, channel; };
    QList<Ch> channels = {
        {"NHK BS1 (S0:Ch17)",         0, 17},
        {"NHK BSプレミアム (S0:Ch0)", 0,  0},
        {"NHK BS (S0:Ch14)",          0, 14},
        {"BS日テレ (S0:Ch1)",         0,  1},
        {"BS朝日 (S0:Ch2)",           0,  2},
        {"BS-TBS (S0:Ch15)",          0, 15},
        {"BSフジ (S0:Ch3)",           0,  3},
        {"BSテレ東 (S0:Ch4)",         0,  4},
        {"WOWOW (S0:Ch18)",           0, 18},
        {"BS11 (S0:Ch8)",             0,  8},
        {"BS12 (S0:Ch10)",            0, 10},
        {"放送大学 (S0:Ch11)",        0, 11},
    };

    m_quickChannelCombo = new QComboBox();
    m_quickChannelCombo->addItem("チャンネルを選択...",
                                 QVariantList({-1, -1, ""}));
    for (const auto &ch : channels)
        m_quickChannelCombo->addItem(
            ch.name,
            QVariantList({ch.space, ch.channel,
                          ch.name.split(" ").first()}));

    m_quickChannelButton = new QPushButton("チャンネル設定");
    m_quickChannelButton->setEnabled(false);

    m_quickChannelLayout->addWidget(new QLabel("放送局:"));
    m_quickChannelLayout->addWidget(m_quickChannelCombo);
    m_quickChannelLayout->addWidget(m_quickChannelButton);
    m_quickChannelLayout->addStretch();

    connect(m_quickChannelCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int i) {
                m_quickChannelButton->setEnabled(i > 0);
            });
    connect(m_quickChannelButton, &QPushButton::clicked,
            this, &MainWindow::onQuickChannelSelected);

    m_quickChannelGroup->setMaximumHeight(80);
    m_mainLayout->addWidget(m_quickChannelGroup);
}

void MainWindow::onQuickChannelSelected()
{
    QVariantList data = m_quickChannelCombo->currentData().toList();
    if (data.size() != 3) return;
    int space = data[0].toInt(), channel = data[1].toInt();
    if (space < 0 || channel < 0) return;

    m_spaceCombo->setCurrentIndex(space);
    m_channelSpin->setValue(channel);

    auto ts = static_cast<BonDriverNetwork::TuningSpace>(space);
    if (m_vlcPlayer->setChannel(ts, static_cast<uint32_t>(channel)))
        addLogMessage(QString("✅ VLC %1 設定成功")
                      .arg(m_quickChannelCombo->currentText()));
    else
        addLogMessage("⚠ VLCチャンネル設定: 接続後に実行してください");
}

void MainWindow::saveSettings()
{
    QSettings s;
    s.setValue("server/host",      m_serverCombo->currentText());
    s.setValue("bondriver/index",  m_bonDriverCombo->currentIndex());
    s.setValue("channel/quick",    m_quickChannelCombo->currentIndex());
    s.setValue("channel/space",    m_spaceCombo->currentIndex());
    s.setValue("channel/channel",  m_channelSpin->value());
}

void MainWindow::restoreSettings()
{
    QSettings s;
    QString savedHost = s.value("server/host", "192.168.0.5").toString();
    int hostIdx = m_serverCombo->findText(savedHost);
    if (hostIdx >= 0)
        m_serverCombo->setCurrentIndex(hostIdx);
    else
        m_serverCombo->setCurrentText(savedHost);
    int bi = s.value("bondriver/index", 0).toInt();
    if (bi >= 0 && bi < m_bonDriverCombo->count())
        m_bonDriverCombo->setCurrentIndex(bi);
    int qi = s.value("channel/quick", 0).toInt();
    if (qi >= 0 && qi < m_quickChannelCombo->count()) {
        // 【修正】シグナル一時切断して不要なチャンネル変更を防ぐ
        m_quickChannelCombo->blockSignals(true);
        m_quickChannelCombo->setCurrentIndex(qi);
        m_quickChannelCombo->blockSignals(false);
        LOG_INFO(QString("設定復元: クイックチャンネル index=%1 (シグナル発火なし)").arg(qi));
    }
    int sp = s.value("channel/space", 0).toInt();
    if (sp >= 0 && sp < m_spaceCombo->count())
        m_spaceCombo->setCurrentIndex(sp);
    m_channelSpin->setValue(s.value("channel/channel", 14).toInt());
    addLogMessage("前回の設定を復元しました");
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // VLCプレイヤーは既にコンストラクタで初期化済み
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        LOG_INFO("showEvent: VLCStreamingPlayer初期化完了");
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // リサイズをffplayに反映（デバウンス: 300ms後に実行）
    static QTimer *resizeTimer = nullptr;
    if (!resizeTimer) {
        resizeTimer = new QTimer(this);
        resizeTimer->setSingleShot(true);
        connect(resizeTimer, &QTimer::timeout, this, [this]() {
            if (m_videoWidget) {
                // VLCプレイヤーは自動的にウィジェットサイズに合わせます
                LOG_DEBUG(QString("ビデオウィジェットサイズ: %1x%2")
                         .arg(m_videoWidget->width())
                         .arg(m_videoWidget->height()));
            }
        });
    }
    resizeTimer->start(300); // 300ms後に実行（連続リサイズを間引く）
}
