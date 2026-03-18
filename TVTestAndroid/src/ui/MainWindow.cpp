#include "MainWindow.h"
#include "utils/Logger.h"
#include <QDateTime>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_network(new BonDriverNetwork(this))
    , m_vlcPlayer(new LibVLCPlayer(this))
    , m_videoWidget(new QWidget(this))
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_updateStatsTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    setupMediaPlayer();
    restoreSettings();
    updateUIState();

    m_updateStatsTimer->setInterval(1000);
    connect(m_updateStatsTimer, &QTimer::timeout,
            this, &MainWindow::onUpdateStatsTimer);

    setWindowTitle("BonDriver Network Player");
    resize(1400, 1000);
    setMinimumSize(1200, 900);

    addLogMessage("アプリケーション開始");
}

MainWindow::~MainWindow()
{
    saveSettings();
    if (m_network->isConnected())
        m_network->disconnectFromServer();
}

void MainWindow::setupMediaPlayer()
{
    // libVLC を映像表示ウィジェットに紐付け
    if (!m_vlcPlayer->init(m_videoWidget->winId())) {
        addLogMessage("❌ libVLC初期化失敗");
    } else {
        addLogMessage("✅ libVLC初期化完了");
    }
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QVBoxLayout(m_centralWidget);

    // 映像表示
    m_videoWidget->setMinimumSize(640, 360);
    m_videoWidget->setStyleSheet("background-color: black;");
    m_mainLayout->addWidget(m_videoWidget);

    // 接続グループ
    m_connectionGroup  = new QGroupBox("サーバー接続");
    m_connectionLayout = new QGridLayout(m_connectionGroup);

    m_connectionLayout->addWidget(new QLabel("サーバー:"), 0, 0);
    m_serverEdit = new QLineEdit("baruma.f5.si");
    m_connectionLayout->addWidget(m_serverEdit, 0, 1);

    m_connectionLayout->addWidget(new QLabel("ポート:"), 0, 2);
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(1192);
    m_connectionLayout->addWidget(m_portSpin, 0, 3);

    m_connectButton = new QPushButton("接続");
    m_connectionLayout->addWidget(m_connectButton, 1, 0);
    m_disconnectButton = new QPushButton("切断");
    m_connectionLayout->addWidget(m_disconnectButton, 1, 1);
    m_connectionStatus = new QLabel("未接続");
    m_connectionLayout->addWidget(m_connectionStatus, 1, 2, 1, 2);
    m_mainLayout->addWidget(m_connectionGroup);

    // BonDriverグループ
    m_bonDriverGroup  = new QGroupBox("BonDriver選択");
    m_bonDriverLayout = new QGridLayout(m_bonDriverGroup);

    m_bonDriverCombo = new QComboBox();
    m_bonDriverCombo->addItem("PT-T (地上波)");
    m_bonDriverCombo->addItem("PT-S (BS/CS)");
    m_bonDriverLayout->addWidget(m_bonDriverCombo, 0, 0);

    m_selectBonDriverButton = new QPushButton("BonDriver選択");
    m_bonDriverLayout->addWidget(m_selectBonDriverButton, 0, 1);
    m_bonDriverStatus = new QLabel("未選択");
    m_bonDriverLayout->addWidget(m_bonDriverStatus, 0, 2);
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
    m_channelLayout->addWidget(m_setChannelButton, 1, 0);
    m_channelStatus = new QLabel("未設定");
    m_channelLayout->addWidget(m_channelStatus, 1, 1, 1, 3);
    m_mainLayout->addWidget(m_channelGroup);

    // TSストリームグループ
    m_streamGroup  = new QGroupBox("TSストリーム");
    m_streamLayout = new QGridLayout(m_streamGroup);

    m_startReceivingButton = new QPushButton("受信開始");
    m_streamLayout->addWidget(m_startReceivingButton, 0, 0);
    m_stopReceivingButton = new QPushButton("受信停止");
    m_streamLayout->addWidget(m_stopReceivingButton, 0, 1);
    m_streamStatus = new QLabel("停止中");
    m_streamLayout->addWidget(m_streamStatus, 0, 2);

    m_streamLayout->addWidget(new QLabel("信号レベル:"), 1, 0);
    m_signalLevelBar = new QProgressBar();
    m_signalLevelBar->setRange(0, 100);
    m_signalLevelBar->setValue(0);
    m_streamLayout->addWidget(m_signalLevelBar, 1, 1, 1, 2);
    m_mainLayout->addWidget(m_streamGroup);

    // 統計グループ
    m_statsGroup  = new QGroupBox("受信統計");
    m_statsLayout = new QGridLayout(m_statsGroup);

    m_totalBytesLabel = new QLabel("総受信量: 0 bytes");
    m_statsLayout->addWidget(m_totalBytesLabel, 0, 0);
    m_packetsLabel = new QLabel("パケット数: 0");
    m_statsLayout->addWidget(m_packetsLabel, 0, 1);
    m_bitrateLabel = new QLabel("ビットレート: 0 Mbps");
    m_statsLayout->addWidget(m_bitrateLabel, 0, 2);
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
    connect(m_connectButton,          &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton,       &QPushButton::clicked,
            this, &MainWindow::onDisconnectClicked);
    connect(m_selectBonDriverButton,  &QPushButton::clicked,
            this, &MainWindow::onSelectBonDriverClicked);
    connect(m_setChannelButton,       &QPushButton::clicked,
            this, &MainWindow::onSetChannelClicked);
    connect(m_startReceivingButton,   &QPushButton::clicked,
            this, &MainWindow::onStartReceivingClicked);
    connect(m_stopReceivingButton,    &QPushButton::clicked,
            this, &MainWindow::onStopReceivingClicked);
    connect(m_clearLogButton,         &QPushButton::clicked,
            this, &MainWindow::onClearLogClicked);

    // ネットワーク
    connect(m_network, &BonDriverNetwork::connected,
            this, &MainWindow::onNetworkConnected);
    connect(m_network, &BonDriverNetwork::disconnected,
            this, &MainWindow::onNetworkDisconnected);
    connect(m_network, &BonDriverNetwork::tsDataReceived,
            this, &MainWindow::onTsDataReceived);
    connect(m_network, &BonDriverNetwork::channelChanged,
            this, &MainWindow::onChannelChanged);
    connect(m_network, &BonDriverNetwork::signalLevelChanged,
            this, &MainWindow::onSignalLevelChanged);
    connect(m_network, &BonDriverNetwork::errorOccurred,
            this, &MainWindow::onErrorOccurred);

    // tsDataReceived → TsBuffer（libVLC へ流す）
    connect(m_network, &BonDriverNetwork::tsDataReceived,
            m_vlcPlayer->tsBuffer(), &TsBuffer::appendData);
}

void MainWindow::onConnectClicked()
{
    QString host = m_serverEdit->text();
    int port = m_portSpin->value();
    addLogMessage(QString("接続開始: %1:%2").arg(host).arg(port));
    if (m_network->connectToServer(host, port))
        addLogMessage("接続成功");
    else
        addLogMessage("接続失敗");
    updateUIState();
}

void MainWindow::onDisconnectClicked()
{
    addLogMessage("切断開始");
    m_vlcPlayer->stop();
    m_network->disconnectFromServer();
    updateUIState();
}

void MainWindow::onSelectBonDriverClicked()
{
    if (!m_network->isConnected()) {
        addLogMessage("❌ 先にサーバーに接続してください");
        return;
    }
    QString bonDriver = (m_bonDriverCombo->currentIndex() == 0) ? "PT-T" : "PT-S";
    addLogMessage(QString("BonDriver選択: %1").arg(bonDriver));
    if (m_network->selectBonDriver(bonDriver)) {
        m_bonDriverStatus->setText(QString("選択済み: %1").arg(bonDriver));
        addLogMessage(QString("✅ BonDriver選択成功: %1").arg(bonDriver));
    } else {
        m_bonDriverStatus->setText("選択失敗");
        addLogMessage("❌ BonDriver選択失敗");
    }
    updateUIState();
    saveSettings();
}

void MainWindow::onSetChannelClicked()
{
    auto space = static_cast<BonDriverNetwork::TuningSpace>(m_spaceCombo->currentIndex());
    uint32_t channel = static_cast<uint32_t>(m_channelSpin->value());
    addLogMessage(QString("チャンネル設定: Space=%1, Channel=%2").arg(space).arg(channel));
    if (m_network->setChannel(space, channel)) {
        addLogMessage("✅ チャンネル設定成功");
        saveSettings();
    } else {
        addLogMessage("❌ チャンネル設定失敗");
    }
    updateUIState();
}

void MainWindow::onStartReceivingClicked()
{
    addLogMessage("TSストリーム受信開始");
    m_network->startReceiving();
    m_streamStatus->setText("受信中");
    m_vlcPlayer->play();

    m_totalBytes      = 0;
    m_totalPackets    = 0;
    m_startTime       = QDateTime::currentDateTime();
    m_lastUpdateTime  = m_startTime;
    m_lastTotalBytes  = 0;
    m_updateStatsTimer->start();
    updateUIState();
}

void MainWindow::onStopReceivingClicked()
{
    addLogMessage("TSストリーム受信停止");
    m_network->stopReceiving();
    m_vlcPlayer->stop();
    m_streamStatus->setText("停止中");
    m_updateStatsTimer->stop();
    updateUIState();
}

void MainWindow::onClearLogClicked()
{
    m_logTextEdit->clear();
}

void MainWindow::onNetworkConnected()
{
    m_connectionStatus->setText("接続済み");
    addLogMessage("ネットワーク接続完了");
    updateUIState();
}

void MainWindow::onNetworkDisconnected()
{
    m_connectionStatus->setText("未接続");
    m_bonDriverStatus->setText("未選択");
    m_channelStatus->setText("未設定");
    m_vlcPlayer->stop();
    addLogMessage("ネットワーク切断");
    updateUIState();
}

void MainWindow::onTsDataReceived(const QByteArray &data)
{
    m_totalBytes += data.size();
    m_totalPackets++;

    // 最初の数パケットのみログ
    if (m_totalPackets <= 3) {
        addLogMessage(QString("✅ TSデータ受信: パケット#%1, %2 bytes")
                      .arg(m_totalPackets).arg(data.size()));
    } else if (m_totalPackets % 1000 == 0) {
        addLogMessage(QString("📊 受信中: パケット#%1").arg(m_totalPackets));
    }
    // libVLC への転送は TsBuffer 経由で自動的に行われる
}

void MainWindow::onChannelChanged(BonDriverNetwork::TuningSpace space, uint32_t channel)
{
    m_channelStatus->setText(QString("Space=%1, Channel=%2").arg(space).arg(channel));
    addLogMessage(QString("チャンネル変更: Space=%1, Channel=%2").arg(space).arg(channel));

    // 古いTSを捨てて再生し直す
    m_vlcPlayer->stop();
    m_vlcPlayer->clearBuffer();

    QTimer::singleShot(300, this, [this]() {
        m_vlcPlayer->play();
        addLogMessage("▶ 再生再開");
    });
}

void MainWindow::onSignalLevelChanged(float level)
{
    m_signalLevelBar->setValue(static_cast<int>(level * 100));
}

void MainWindow::onErrorOccurred(const QString &error)
{
    addLogMessage(QString("❌ エラー: %1").arg(error));
}

void MainWindow::onUpdateStatsTimer()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 lastUpdateMs = m_lastUpdateTime.msecsTo(currentTime);

    m_totalBytesLabel->setText(
        QString("総受信量: %1 MB")
        .arg(m_totalBytes / (1024.0 * 1024.0), 0, 'f', 2));
    m_packetsLabel->setText(QString("パケット数: %1").arg(m_totalPackets));

    if (lastUpdateMs > 0) {
        qint64 deltaBytes = m_totalBytes - m_lastTotalBytes;
        double bitrate = (deltaBytes * 8.0 * 1000.0) / lastUpdateMs / (1024.0 * 1024.0);
        m_bitrateLabel->setText(
            QString("ビットレート: %1 Mbps").arg(bitrate, 0, 'f', 2));
    }

    m_lastUpdateTime = currentTime;
    m_lastTotalBytes = m_totalBytes;
}

void MainWindow::addLogMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logTextEdit->append(QString("[%1] %2").arg(timestamp, message));

    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logTextEdit->setTextCursor(cursor);

    QApplication::processEvents();
}

void MainWindow::updateUIState()
{
    bool connected = m_network->isConnected();
    bool bonDriverSelected = connected &&
                             m_bonDriverStatus->text() != "未選択" &&
                             m_bonDriverStatus->text() != "選択失敗";

    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_selectBonDriverButton->setEnabled(connected);
    m_bonDriverCombo->setEnabled(true);
    m_setChannelButton->setEnabled(bonDriverSelected);
    m_startReceivingButton->setEnabled(bonDriverSelected);
    m_stopReceivingButton->setEnabled(bonDriverSelected);

    if (m_quickChannelGroup)
        m_quickChannelGroup->setEnabled(bonDriverSelected);
}

void MainWindow::setupQuickChannelSelection()
{
    m_quickChannelGroup  = new QGroupBox("クイックチャンネル選択");
    m_quickChannelLayout = new QHBoxLayout(m_quickChannelGroup);

    struct ChannelInfo { QString name; int space; int channel; };
    QList<ChannelInfo> channels = {
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
    for (const auto &ch : channels) {
        m_quickChannelCombo->addItem(
            ch.name,
            QVariantList({ch.space, ch.channel,
                          ch.name.split(" ").first()}));
    }

    m_quickChannelButton = new QPushButton("チャンネル設定");
    m_quickChannelButton->setEnabled(false);

    m_quickChannelLayout->addWidget(new QLabel("放送局:"));
    m_quickChannelLayout->addWidget(m_quickChannelCombo);
    m_quickChannelLayout->addWidget(m_quickChannelButton);
    m_quickChannelLayout->addStretch();

    connect(m_quickChannelCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                m_quickChannelButton->setEnabled(index > 0);
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

    int space   = data[0].toInt();
    int channel = data[1].toInt();
    if (space < 0 || channel < 0) return;

    addLogMessage(QString("クイックチャンネル: %1")
                  .arg(m_quickChannelCombo->currentText()));

    m_spaceCombo->setCurrentIndex(space);
    m_channelSpin->setValue(channel);

    auto tuningSpace = static_cast<BonDriverNetwork::TuningSpace>(space);
    if (m_network->setChannel(tuningSpace, static_cast<uint32_t>(channel)))
        addLogMessage("✅ チャンネル設定成功");
    else
        addLogMessage("❌ チャンネル設定失敗");
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("bondriver/selectedIndex",
                      m_bonDriverCombo->currentIndex());
    settings.setValue("channel/selectedQuickIndex",
                      m_quickChannelCombo->currentIndex());
    settings.setValue("channel/space",   m_spaceCombo->currentIndex());
    settings.setValue("channel/channel", m_channelSpin->value());
}

void MainWindow::restoreSettings()
{
    QSettings settings;

    int bonIdx = settings.value("bondriver/selectedIndex", 0).toInt();
    if (bonIdx >= 0 && bonIdx < m_bonDriverCombo->count())
        m_bonDriverCombo->setCurrentIndex(bonIdx);

    int quickIdx = settings.value("channel/selectedQuickIndex", 0).toInt();
    if (quickIdx >= 0 && quickIdx < m_quickChannelCombo->count())
        m_quickChannelCombo->setCurrentIndex(quickIdx);

    int space = settings.value("channel/space", 0).toInt();
    if (space >= 0 && space < m_spaceCombo->count())
        m_spaceCombo->setCurrentIndex(space);

    m_channelSpin->setValue(settings.value("channel/channel", 14).toInt());

    addLogMessage("前回の設定を復元しました");
}
