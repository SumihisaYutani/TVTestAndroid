#include "MainWindow.h"
#include "media/DirectStreamPlayer.h"
#include "utils/Logger.h"
#ifdef USE_FFMPEG
#include "media/FFmpegDecoder.h"
#include "ui/FFmpegVideoWidget.h"
#endif
#include <QDateTime>
#include <QApplication>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_network(new BonDriverNetwork(this))
    , m_centralWidget(nullptr)
    , m_updateStatsTimer(new QTimer(this))
    , m_totalBytes(0)
    , m_totalPackets(0)
    , m_videoWidget(new QVideoWidget(this))
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_tsBuffer(new QBuffer(&m_tsData, this))
#ifdef USE_FFMPEG
    , m_ffmpegDecoder(new FFmpegDecoder(this))
    , m_ffmpegVideoWidget(new FFmpegVideoWidget(this))
#endif
    , m_directStreamPlayer(new DirectStreamPlayer(this))
    , m_directVideoWidget(new QVideoWidget(this))
{
    setupUI();
    setupConnections();
    setupMediaPlayer();
    setupDirectStreamPlayer();
    restoreSettings();  // 前回の設定を復元
    updateUIState();
    
    // 統計更新タイマー設定（1秒間隔）
    m_updateStatsTimer->setInterval(1000);
    connect(m_updateStatsTimer, &QTimer::timeout, this, &MainWindow::onUpdateStatsTimer);
    
    setWindowTitle("TVTest Android - BonDriver_Proxy接続テスト");
    
    // フルHD環境（1920x1080）に最適化されたウィンドウサイズ設定
    // ボタン重なり防止のため、より大きなウィンドウサイズを設定
    resize(1400, 1000);
    setMinimumSize(1200, 900);
    
    addLogMessage("アプリケーション開始");
}

MainWindow::~MainWindow()
{
    saveSettings();  // 設定を保存
    
    if (m_network->isConnected()) {
        m_network->disconnectFromServer();
    }
}

void MainWindow::setupMediaPlayer()
{
    // Qt Multimedia セットアップ
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_mediaPlayer->setVideoOutput(m_videoWidget);
    
    // TSデータバッファの設定
    m_tsBuffer->open(QIODevice::ReadOnly);
    
    // MediaPlayerエラー監視
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &errorString) {
        addLogMessage(QString("❌ MediaPlayerエラー: %1 (%2)").arg(errorString).arg(static_cast<int>(error)));
    });
    
    // MediaPlayer状態変更監視
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        QString stateStr;
        switch (state) {
            case QMediaPlayer::StoppedState: stateStr = "停止"; break;
            case QMediaPlayer::PlayingState: stateStr = "再生中"; break;
            case QMediaPlayer::PausedState: stateStr = "一時停止"; break;
        }
        addLogMessage(QString("📺 MediaPlayer状態: %1").arg(stateStr));
    });
    
    // MediaStatus変更監視
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        QString statusStr;
        switch (status) {
            case QMediaPlayer::NoMedia: statusStr = "メディアなし"; break;
            case QMediaPlayer::LoadingMedia: statusStr = "ロード中"; break;
            case QMediaPlayer::LoadedMedia: statusStr = "ロード完了"; break;
            case QMediaPlayer::StalledMedia: statusStr = "停滞中"; break;
            case QMediaPlayer::BufferingMedia: statusStr = "バッファリング中"; break;
            case QMediaPlayer::BufferedMedia: statusStr = "バッファリング完了"; break;
            case QMediaPlayer::EndOfMedia: statusStr = "再生終了"; break;
            case QMediaPlayer::InvalidMedia: statusStr = "無効なメディア"; break;
        }
        addLogMessage(QString("📊 MediaStatus: %1").arg(statusStr));
        
        // 無効なメディアの場合、詳細なエラー情報を出力
        if (status == QMediaPlayer::InvalidMedia) {
            addLogMessage(QString("❌ TSストリーム再生失敗: %1").arg(m_mediaPlayer->errorString()));
            addLogMessage("💡 Qt MultimediaはこのTSストリーム形式に対応していません");
            addLogMessage("💡 解決策: VLCプレイヤーなどの外部プレイヤーで再生してください");
            
            // 外部プレイヤー起動を提案
            QString tempTsFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/tvtest_live_stream.ts";
            addLogMessage(QString("📂 TSファイル場所: %1").arg(tempTsFile));
        }
        
        // ロード完了時の情報
        if (status == QMediaPlayer::LoadedMedia) {
            addLogMessage("✅ TSストリーム読み込み成功！");
            addLogMessage(QString("📺 画面サイズ: %1x%2")
                         .arg(m_videoWidget->width())
                         .arg(m_videoWidget->height()));
        }
    });
    
    // MediaPlayerエラー監視
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &errorString) {
        QString errorTypeStr;
        switch (error) {
            case QMediaPlayer::NoError: errorTypeStr = "エラーなし"; break;
            case QMediaPlayer::ResourceError: errorTypeStr = "リソースエラー"; break;
            case QMediaPlayer::FormatError: errorTypeStr = "フォーマットエラー"; break;
            case QMediaPlayer::NetworkError: errorTypeStr = "ネットワークエラー"; break;
            case QMediaPlayer::AccessDeniedError: errorTypeStr = "アクセス拒否エラー"; break;
        }
        addLogMessage(QString("❌ MediaPlayerエラー: %1 - %2").arg(errorTypeStr).arg(errorString));
    });
    
    // デュレーション変更監視（TSファイルの長さを確認）
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        if (duration > 0) {
            addLogMessage(QString("⏱️ メディアの長さ: %1 秒").arg(duration / 1000.0, 0, 'f', 1));
        }
    });
    
#ifdef USE_FFMPEG
    // FFmpegDecoder統合
    connect(m_ffmpegDecoder, &FFmpegDecoder::frameReady, 
            m_ffmpegVideoWidget, &FFmpegVideoWidget::displayFrame);
    
    connect(m_ffmpegDecoder, &FFmpegDecoder::errorOccurred, this, [this](const QString &error) {
        addLogMessage(QString("❌ FFmpegDecoderエラー: %1").arg(error));
    });
    
    connect(m_ffmpegDecoder, &FFmpegDecoder::statsUpdated, this, [this](const FFmpegDecoder::DecodeStats &stats) {
        addLogMessage(QString("📊 FFmpeg統計: フレーム=%1, FPS=%2, コーデック=%3")
                      .arg(stats.totalFrames)
                      .arg(stats.frameRate, 0, 'f', 1)
                      .arg(stats.codecName));
    });
    
    connect(m_ffmpegVideoWidget, &FFmpegVideoWidget::frameDisplayed, this, [this]() {
        // フレーム表示完了時の処理（必要に応じて）
    });
    
    addLogMessage("Qt Multimedia + FFmpeg初期化完了");
#else
    addLogMessage("Qt Multimedia初期化完了 (FFmpegなし)");
#endif
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    
#ifdef USE_FFMPEG
    // 動画表示ウィジェット（FFmpeg版を優先使用） - サイズ縮小
    m_ffmpegVideoWidget->setMinimumSize(320, 180);  // サイズを半分に
    m_ffmpegVideoWidget->setMaximumSize(640, 360);  // 最大サイズ制限
    m_ffmpegVideoWidget->setStyleSheet("background-color: black;");
    m_mainLayout->addWidget(m_ffmpegVideoWidget);
    
    // Qt Multimedia版は非表示（デバッグ用に保持）
    m_videoWidget->setVisible(false);
#else
    // FFmpegが無い場合はQt Multimedia版を使用 - サイズ縮小
    m_videoWidget->setMinimumSize(320, 180);  // サイズを半分に
    m_videoWidget->setMaximumSize(640, 360);  // 最大サイズ制限
    m_videoWidget->setStyleSheet("background-color: black;");
    m_mainLayout->addWidget(m_videoWidget);
#endif

    // 直接ストリーミング再生ウィジェット（元TVTest方式） - サイズ縮小
    m_directVideoWidget->setMinimumSize(320, 180);  // サイズを半分に
    m_directVideoWidget->setMaximumSize(640, 360);  // 最大サイズ制限
    m_directVideoWidget->setStyleSheet("background-color: black; border: 2px solid blue;");
    m_directVideoWidget->setVisible(false); // デフォルトは非表示
    m_mainLayout->addWidget(m_directVideoWidget);
    
    // 接続グループ
    m_connectionGroup = new QGroupBox("サーバー接続");
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
    m_bonDriverGroup = new QGroupBox("BonDriver選択");
    m_bonDriverLayout = new QGridLayout(m_bonDriverGroup);
    
    m_bonDriverCombo = new QComboBox();
    m_bonDriverCombo->addItem("PT-T (地上波)");  // サーバー設定の00=PT-Tに対応
    m_bonDriverCombo->addItem("PT-S (BS/CS)");  // サーバー設定の01=PT-Sに対応
    m_bonDriverLayout->addWidget(m_bonDriverCombo, 0, 0);
    
    m_selectBonDriverButton = new QPushButton("BonDriver選択");
    m_bonDriverLayout->addWidget(m_selectBonDriverButton, 0, 1);
    
    // 初期状態でBonDriverコンボボックスは有効に設定
    m_bonDriverCombo->setEnabled(true);
    
    // BonDriverグループ自体も確実に有効化
    m_bonDriverGroup->setEnabled(true);
    
    m_bonDriverStatus = new QLabel("未選択");
    m_bonDriverLayout->addWidget(m_bonDriverStatus, 0, 2);
    
    m_mainLayout->addWidget(m_bonDriverGroup);
    
    // チャンネルグループ
    m_channelGroup = new QGroupBox("チャンネル設定");
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
    m_channelSpin->setValue(14); // Wiresharkログ解析結果: Channel=14（実際に放送があるチャンネル）
    m_channelLayout->addWidget(m_channelSpin, 0, 3);
    
    m_setChannelButton = new QPushButton("チャンネル設定");
    m_channelLayout->addWidget(m_setChannelButton, 1, 0);
    
    m_channelStatus = new QLabel("未設定");
    m_channelLayout->addWidget(m_channelStatus, 1, 1, 1, 3);
    
    m_mainLayout->addWidget(m_channelGroup);
    
    // TSストリームグループ
    m_streamGroup = new QGroupBox("TSストリーム");
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
    
    // 直接ストリーミンググループ（元TVTest方式）
    m_directStreamGroup = new QGroupBox("直接ストリーミング再生");
    m_directStreamLayout = new QHBoxLayout(m_directStreamGroup);
    
    m_startDirectStreamButton = new QPushButton("直接再生開始");
    m_startDirectStreamButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    m_directStreamLayout->addWidget(m_startDirectStreamButton);
    
    m_stopDirectStreamButton = new QPushButton("直接再生停止");
    m_stopDirectStreamButton->setStyleSheet("background-color: #f44336; color: white; font-weight: bold;");
    m_directStreamLayout->addWidget(m_stopDirectStreamButton);
    
    m_directStreamStatus = new QLabel("停止中");
    m_directStreamStatus->setStyleSheet("font-weight: bold;");
    m_directStreamLayout->addWidget(m_directStreamStatus);
    
    m_bufferStatus = new QLabel("バッファ: 0 bytes");
    m_bufferStatus->setStyleSheet("color: #2196F3;");
    m_directStreamLayout->addWidget(m_bufferStatus);
    
    m_mainLayout->addWidget(m_directStreamGroup);
    
    // 統計グループ
    m_statsGroup = new QGroupBox("受信統計");
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
    m_logTextEdit->setMaximumHeight(150); // フルHD環境でコンパクトに
    m_logTextEdit->setReadOnly(true);
    m_mainLayout->addWidget(m_logTextEdit);
    
    m_clearLogButton = new QPushButton("ログクリア");
    m_mainLayout->addWidget(m_clearLogButton);
    
    // TVTestクイックチャンネル選択（.ch2ファイルベース）
    setupQuickChannelSelection();
}

void MainWindow::setupConnections()
{
    // UIボタン
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_selectBonDriverButton, &QPushButton::clicked, this, &MainWindow::onSelectBonDriverClicked);
    connect(m_setChannelButton, &QPushButton::clicked, this, &MainWindow::onSetChannelClicked);
    connect(m_startReceivingButton, &QPushButton::clicked, this, &MainWindow::onStartReceivingClicked);
    connect(m_stopReceivingButton, &QPushButton::clicked, this, &MainWindow::onStopReceivingClicked);
    connect(m_startDirectStreamButton, &QPushButton::clicked, this, &MainWindow::onStartDirectStreamClicked);
    connect(m_stopDirectStreamButton, &QPushButton::clicked, this, &MainWindow::onStopDirectStreamClicked);
    connect(m_clearLogButton, &QPushButton::clicked, this, &MainWindow::onClearLogClicked);
    
    // ネットワークシグナル
    connect(m_network, &BonDriverNetwork::connected, this, &MainWindow::onNetworkConnected);
    connect(m_network, &BonDriverNetwork::disconnected, this, &MainWindow::onNetworkDisconnected);
    connect(m_network, &BonDriverNetwork::tsDataReceived, this, &MainWindow::onTsDataReceived);
    connect(m_network, &BonDriverNetwork::channelChanged, this, &MainWindow::onChannelChanged);
    connect(m_network, &BonDriverNetwork::signalLevelChanged, this, &MainWindow::onSignalLevelChanged);
    connect(m_network, &BonDriverNetwork::errorOccurred, this, &MainWindow::onErrorOccurred);
}

void MainWindow::onConnectClicked()
{
    QString host = m_serverEdit->text();
    int port = m_portSpin->value();
    
    addLogMessage(QString("接続開始: %1:%2").arg(host).arg(port));
    
    if (m_network->connectToServer(host, port)) {
        addLogMessage("接続成功");
    } else {
        addLogMessage("接続失敗");
    }
    
    updateUIState();
}

void MainWindow::onDisconnectClicked()
{
    addLogMessage("切断開始");
    m_network->disconnectFromServer();
    updateUIState();
}

void MainWindow::onSelectBonDriverClicked()
{
    // 接続状態確認
    if (!m_network->isConnected()) {
        addLogMessage("❌ エラー: サーバーに接続してからBonDriverを選択してください");
        return;
    }
    
    QString bonDriver;
    int index = m_bonDriverCombo->currentIndex();
    
    if (index == 0) {
        bonDriver = "PT-T";  // サーバーの00=PT-Tに対応
    } else {
        bonDriver = "PT-S";  // サーバーの01=PT-Sに対応
    }
    
    addLogMessage(QString("=== BonDriver選択開始: %1 ===").arg(bonDriver));
    addLogMessage(QString("コンボボックスインデックス: %1").arg(index));
    addLogMessage(QString("選択BonDriver: %1").arg(bonDriver));
    
    if (m_network->selectBonDriver(bonDriver)) {
        m_bonDriverStatus->setText(QString("選択済み: %1").arg(bonDriver));
        addLogMessage(QString("✅ BonDriver選択成功: %1").arg(bonDriver));
        addLogMessage(QString("ステータス更新: %1").arg(m_bonDriverStatus->text()));
    } else {
        m_bonDriverStatus->setText("選択失敗");
        addLogMessage(QString("❌ BonDriver選択失敗: %1").arg(bonDriver));
    }
    
    addLogMessage("UI状態更新を実行");
    updateUIState();
    
    // 設定を自動保存
    saveSettings();
    
    addLogMessage("=== BonDriver選択処理完了 ===");
}

void MainWindow::onSetChannelClicked()
{
    BonDriverNetwork::TuningSpace space = static_cast<BonDriverNetwork::TuningSpace>(m_spaceCombo->currentIndex());
    uint32_t channel = static_cast<uint32_t>(m_channelSpin->value());
    
    addLogMessage(QString("チャンネル設定: Space=%1, Channel=%2").arg(space).arg(channel));
    
    if (m_network->setChannel(space, channel)) {
        addLogMessage(QString("チャンネル設定成功: Space=%1, Channel=%2").arg(space).arg(channel));
        
        // 設定を自動保存
        saveSettings();
    } else {
        addLogMessage(QString("チャンネル設定失敗: Space=%1, Channel=%2").arg(space).arg(channel));
    }
    
    updateUIState();
}

void MainWindow::onStartReceivingClicked()
{
    addLogMessage("TSストリーム受信開始");
    m_network->startReceiving();
    m_streamStatus->setText("受信中");
    
    // 統計リセット
    m_totalBytes = 0;
    m_totalPackets = 0;
    m_startTime = QDateTime::currentDateTime();
    m_lastUpdateTime = m_startTime;
    m_lastTotalBytes = 0;
    
    m_updateStatsTimer->start();
    updateUIState();
}

void MainWindow::onStopReceivingClicked()
{
    addLogMessage("TSストリーム受信停止");
    m_network->stopReceiving();
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
    addLogMessage("ネットワーク切断");
    updateUIState();
}

void MainWindow::onTsDataReceived(const QByteArray &data)
{
    m_totalBytes += data.size();
    m_totalPackets++;
    
    // メモリ使用量制限（最大5MB蓄積）
    const int maxTsBufferSize = 5 * 1024 * 1024; // 5MB
    if (m_tsData.size() + data.size() > maxTsBufferSize) {
        // 古いデータを半分削除
        int removeSize = m_tsData.size() / 2;
        m_tsData.remove(0, removeSize);
    }
    
    // TSデータをバッファに蓄積
    m_tsData.append(data);
    
    // ログ出力を大幅に削減
    if (m_totalPackets <= 3) {
        addLogMessage(QString("✅ TSデータ受信開始: パケット#%1, %2 bytes")
                      .arg(m_totalPackets).arg(data.size()));
    }
    else if (m_totalPackets % 1000 == 0) {
        addLogMessage(QString("📊 受信統計: パケット#%1, 蓄積%2 KB")
                      .arg(m_totalPackets).arg(m_tsData.size() / 1024));
    }
    
#ifdef USE_FFMPEG
    // FFmpegDecoderでリアルタイムTSデコード・再生
    if (m_ffmpegDecoder) {
        // TSデータをFFmpegDecoderに送信
        m_ffmpegDecoder->inputTsData(data);
        
        // 統計情報の定期報告
        if (m_totalPackets % 100 == 0) {
            addLogMessage(QString("📊 FFmpeg処理: 総サイズ %1 KB, パケット#%2")
                          .arg(m_tsData.size() / 1024)
                          .arg(m_totalPackets));
        }
    }
#else
    // Qt MultimediaでTSストリーム直接再生を試行
    static bool mediaPlayerTested = false;
    const int minBufferSize = 188 * 200; // TSパケット200個分（約37KB）
    
    if (m_tsData.size() >= minBufferSize && !mediaPlayerTested) {
        QString tempTsFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/tvtest_live_stream.ts";
        QFile tsFile(tempTsFile);
        
        if (tsFile.open(QIODevice::WriteOnly)) {
            tsFile.write(m_tsData);
            tsFile.close();
            
            addLogMessage(QString("📁 TSファイル作成: %1 (%2 KB)").arg(tempTsFile).arg(m_tsData.size() / 1024));
            
            // Qt MultimediaでTSファイル直接再生を試行
            QUrl tsUrl = QUrl::fromLocalFile(tempTsFile);
            m_mediaPlayer->setSource(tsUrl);
            m_mediaPlayer->play();
            
            mediaPlayerTested = true;
            addLogMessage("🎬 Qt MultimediaでTSストリーム再生試行中...");
            addLogMessage("📺 画面に映像が表示されない場合は外部プレイヤーを使用してください");
        }
    }
    
    // 継続的にTSファイルを更新（ライブストリーミング風）
    if (mediaPlayerTested && m_totalPackets % 500 == 0) {
        QString tempTsFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/tvtest_live_stream.ts";
        QFile tsFile(tempTsFile);
        
        if (tsFile.open(QIODevice::WriteOnly)) {
            tsFile.write(m_tsData);
            tsFile.close();
            
            // MediaPlayerに更新を通知
            if (m_mediaPlayer->playbackState() != QMediaPlayer::PlayingState) {
                QUrl tsUrl = QUrl::fromLocalFile(tempTsFile);
                m_mediaPlayer->setSource(tsUrl);
                m_mediaPlayer->play();
                addLogMessage(QString("🔄 TSストリーム更新: %1 KB").arg(m_tsData.size() / 1024));
            }
        }
    }
#endif
}

void MainWindow::onChannelChanged(BonDriverNetwork::TuningSpace space, uint32_t channel)
{
    m_channelStatus->setText(QString("Space=%1, Channel=%2").arg(space).arg(channel));
    addLogMessage(QString("チャンネル変更完了: Space=%1, Channel=%2").arg(space).arg(channel));
    
    // 🔄 チャンネル変更時: DirectStreamPlayerの再生状態をリセット
    addLogMessage("🔄 チャンネル変更: 再生状態リセット実行");
    m_directStreamPlayer->resetForChannelChange();
    
    // 🎯 チャンネル変更完了後、自動的に直接ストリーミングを開始
    addLogMessage("📺 チャンネル変更完了 → 自動的に直接ストリーミング開始");
    QTimer::singleShot(500, this, [this]() {
        onStartDirectStreamClicked();
    });
}

void MainWindow::onSignalLevelChanged(float level)
{
    m_signalLevelBar->setValue(static_cast<int>(level * 100));
    
    // 最初の数回のみログ出力
    static int signalLogCount = 0;
    if (signalLogCount < 3) {
        addLogMessage(QString("信号レベル: %1%").arg(level * 100, 0, 'f', 1));
        signalLogCount++;
    }
}

void MainWindow::onErrorOccurred(const QString &error)
{
    addLogMessage(QString("エラー: %1").arg(error));
}

void MainWindow::onUpdateStatsTimer()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 elapsedMs = m_startTime.msecsTo(currentTime);
    qint64 lastUpdateMs = m_lastUpdateTime.msecsTo(currentTime);
    
    // 統計更新
    m_totalBytesLabel->setText(QString("総受信量: %1 MB").arg(m_totalBytes / (1024.0 * 1024.0), 0, 'f', 2));
    m_packetsLabel->setText(QString("パケット数: %1").arg(m_totalPackets));
    
    // ビットレート計算（直近1秒間）
    if (lastUpdateMs > 0) {
        qint64 deltaBytes = m_totalBytes - m_lastTotalBytes;
        double bitrate = (deltaBytes * 8.0 * 1000.0) / lastUpdateMs / (1024.0 * 1024.0); // Mbps
        m_bitrateLabel->setText(QString("ビットレート: %1 Mbps").arg(bitrate, 0, 'f', 2));
        
        // 📊 統計ログをファイルに出力（10秒ごと）
        static int statsLogCounter = 0;
        statsLogCounter++;
        if (statsLogCounter % 10 == 0) {
            LOG_INFO(QString("📊 [統計] 総受信: %1 MB | パケット: %2 | ビットレート: %3 Mbps | 経過: %4 分")
                    .arg(m_totalBytes / (1024.0 * 1024.0), 0, 'f', 2)
                    .arg(m_totalPackets)
                    .arg(bitrate, 0, 'f', 2)
                    .arg(elapsedMs / 60000.0, 0, 'f', 1));
        }
    }
    
    m_lastUpdateTime = currentTime;
    m_lastTotalBytes = m_totalBytes;
}

void MainWindow::addLogMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logLine = QString("[%1] %2").arg(timestamp, message);
    
    m_logTextEdit->append(logLine);
    
    // 自動スクロール
    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logTextEdit->setTextCursor(cursor);
    
    // アプリケーション処理
    QApplication::processEvents();
}

void MainWindow::updateUIState()
{
    bool connected = m_network->isConnected();
    bool bonDriverSelected = connected && !m_bonDriverStatus->text().isEmpty() && 
                            m_bonDriverStatus->text() != "未選択";
    
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    
    // BonDriver controls - ensure the group is always enabled
    m_bonDriverGroup->setEnabled(true);
    m_bonDriverCombo->setEnabled(true);  // コンボボックスは常に有効（選択可能）
    m_selectBonDriverButton->setEnabled(true);  // 常に有効（接続前でも選択可能）
    m_setChannelButton->setEnabled(bonDriverSelected);  // BonDriver選択後に有効化
    m_startReceivingButton->setEnabled(bonDriverSelected);  // BonDriver選択後に有効化
    m_stopReceivingButton->setEnabled(bonDriverSelected);   // BonDriver選択後に有効化
    
    // 直接ストリーミングボタンの制御
    m_startDirectStreamButton->setEnabled(bonDriverSelected);
    m_stopDirectStreamButton->setEnabled(bonDriverSelected);
    
    // クイックチャンネル選択も BonDriver選択後に有効化
    if (m_quickChannelGroup) {
        m_quickChannelGroup->setEnabled(bonDriverSelected);
    }
    
}

void MainWindow::setupQuickChannelSelection()
{
    m_quickChannelGroup = new QGroupBox("クイックチャンネル選択（TVTest実チャンネル）");
    m_quickChannelLayout = new QHBoxLayout(m_quickChannelGroup);
    
    // TVTest BonDriver_Proxy_S.ch2 から抽出した確実に放送があるチャンネル
    struct ChannelInfo {
        QString name;
        int space;
        int channel;
    };
    
    // BS（Space=0）の主要チャンネル
    QList<ChannelInfo> channels = {
        {"NHK BS1 (S0:Ch17)", 0, 17},          // ServiceID=101, 確実に放送あり
        {"NHK BSプレミアム (S0:Ch0)", 0, 0},     // ServiceID=151, 確実に放送あり  
        {"NHK BS (S0:Ch14)", 0, 14},           // ServiceID=141, 確実に放送あり（Wireshark使用チャンネル）
        {"BS日テレ (S0:Ch1)", 0, 1},           // ServiceID=161
        {"BS朝日 (S0:Ch2)", 0, 2},             // ServiceID=171
        {"BS-TBS (S0:Ch15)", 0, 15},           // ServiceID=181
        {"BSフジ (S0:Ch3)", 0, 3},             // ServiceID=191
        {"BS テレ東 (S0:Ch4)", 0, 4},          // ServiceID=191
        {"WOWOW プライム (S0:Ch18)", 0, 18},   // ServiceID=200
        {"BS11 (S0:Ch8)", 0, 8},               // ServiceID=211
        {"BS12 (S0:Ch10)", 0, 10},             // ServiceID=222
        {"放送大学 (S0:Ch11)", 0, 11},          // ServiceID=231
    };
    
    // プルダウンメニュー作成
    m_quickChannelCombo = new QComboBox();
    m_quickChannelCombo->addItem("チャンネルを選択してください...", QVariantList({-1, -1, ""}));
    
    for (const auto &ch : channels) {
        QVariantList channelData = {ch.space, ch.channel, ch.name.split(" ").first()};
        m_quickChannelCombo->addItem(ch.name, channelData);
    }
    
    // 選択ボタン
    m_quickChannelButton = new QPushButton("チャンネル設定");
    m_quickChannelButton->setEnabled(false); // 初期は無効
    
    // レイアウトに追加
    m_quickChannelLayout->addWidget(new QLabel("放送局:"));
    m_quickChannelLayout->addWidget(m_quickChannelCombo);
    m_quickChannelLayout->addWidget(m_quickChannelButton);
    m_quickChannelLayout->addStretch(); // 右側にスペーサー
    
    // シグナル接続
    connect(m_quickChannelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, [this](int index) {
                m_quickChannelButton->setEnabled(index > 0);
            });
    connect(m_quickChannelButton, &QPushButton::clicked, this, &MainWindow::onQuickChannelSelected);
    
    // コンパクトな高さ設定
    m_quickChannelGroup->setMaximumHeight(80);
    
    m_mainLayout->addWidget(m_quickChannelGroup);
}

void MainWindow::onQuickChannelSelected()
{
    QVariantList channelData = m_quickChannelCombo->currentData().toList();
    if (channelData.size() != 3) return;
    
    int space = channelData[0].toInt();
    int channel = channelData[1].toInt();
    QString channelName = channelData[2].toString();
    
    if (space < 0 || channel < 0) return; // 無効な選択
    
    addLogMessage(QString("=== クイックチャンネル選択: %1 ===").arg(m_quickChannelCombo->currentText()));
    addLogMessage(QString("Space=%1, Channel=%2").arg(space).arg(channel));
    
    // 元のUIの選択も更新
    m_spaceCombo->setCurrentIndex(space);
    m_channelSpin->setValue(channel);
    
    // 実際のチャンネル設定実行
    BonDriverNetwork::TuningSpace tuningSpace = static_cast<BonDriverNetwork::TuningSpace>(space);
    if (m_network->setChannel(tuningSpace, static_cast<uint32_t>(channel))) {
        addLogMessage(QString("✅ %1 チャンネル設定成功").arg(channelName));
    } else {
        addLogMessage(QString("❌ %1 チャンネル設定失敗").arg(channelName));
    }
}

// DirectStreamPlayer関連メソッド実装

void MainWindow::setupDirectStreamPlayer()
{
    addLogMessage("=== setupDirectStreamPlayer 開始 ===");
    
    // DirectStreamPlayerの初期化
    addLogMessage("Step 1: DirectStreamPlayer VideoWidget設定");
    m_directStreamPlayer->setVideoWidget(m_directVideoWidget);
    addLogMessage("Step 1: COMPLETED");
    
    // BonDriverNetworkとDirectStreamPlayerの連携設定
    addLogMessage("Step 2: BonDriverNetwork連携設定");
    m_network->setDirectStreamPlayer(m_directStreamPlayer);
    addLogMessage("Step 2: COMPLETED");
    
    // DirectStreamPlayerのシグナル接続
    addLogMessage("Step 3: DirectStreamPlayerシグナル接続");
    connect(m_directStreamPlayer, &DirectStreamPlayer::playbackStateChanged,
            this, &MainWindow::onDirectStreamPlaybackStateChanged);
    connect(m_directStreamPlayer, &DirectStreamPlayer::mediaInfoChanged,
            this, &MainWindow::onDirectStreamMediaInfoChanged);
    connect(m_directStreamPlayer, &DirectStreamPlayer::errorOccurred,
            this, &MainWindow::onDirectStreamErrorOccurred);
    connect(m_directStreamPlayer, &DirectStreamPlayer::bufferStatusChanged,
            this, &MainWindow::onDirectStreamBufferStatusChanged);
    addLogMessage("Step 3: COMPLETED");

#ifdef USE_FFMPEG
    addLogMessage("Step 4: FFmpeg TSデータ転送設定");
    // DirectStreamPlayerからFFmpegDecoderへのTSデータ転送設定
    connect(m_network, &BonDriverNetwork::directTsDataReceived,
            this, [this](const QByteArray &data) {
                if (m_ffmpegDecoder) {
                    m_ffmpegDecoder->inputTsData(data);
                }
            });
    addLogMessage("DirectStreamPlayer → FFmpegDecoder データ転送設定完了");
#else
    addLogMessage("Step 4: TSファイル保存システム設定");
    // FFmpeg無効時: 安定したTSファイル保存と再生
    connect(m_network, &BonDriverNetwork::directTsDataReceived,
            this, [this](const QByteArray &data) {
                static QFile tsFile("received_stream.ts");
                static bool fileOpened = false;
                static bool playbackStarted = false;
                static int packetCount = 0;
                static qint64 totalBytes = 0;
                
                // 1. TSファイルに保存
                if (!fileOpened) {
                    tsFile.open(QIODevice::WriteOnly);
                    fileOpened = true;
                    addLogMessage("📁 TSファイル保存開始: received_stream.ts");
                }
                tsFile.write(data);
                
                // 2. DirectStreamPlayerにも送信
                m_directStreamPlayer->addTsStream(data);
                
                packetCount++;
                totalBytes += data.size();
                
                // 3. 十分なデータが蓄積されたら外部プレイヤーで再生開始
                if (!playbackStarted && totalBytes >= 2 * 1024 * 1024) { // 2MB蓄積
                    playbackStarted = true;
                    tsFile.flush(); // データをフラッシュ
                    
                    addLogMessage(QString("🎥 TSファイル再生開始: %1KB蓄積完了").arg(totalBytes/1024));
                    
                    // FFplayで外部再生
                    QTimer::singleShot(1000, this, [this]() {
                        QString filePath = QFileInfo("received_stream.ts").absoluteFilePath();
                        QString ffplayPath = "C:/ffmpeg/bin/ffplay.exe";
                        
                        if (QFile::exists(ffplayPath) && QFile::exists(filePath)) {
                            addLogMessage(QString("🎬 FFplay外部再生開始: %1").arg(filePath));
                            
                            // FFplayで再生（リアルタイムストリーミングオプション付き）
                            QStringList arguments;
                            arguments << "-f" << "mpegts";        // MPEG-TSフォーマット指定
                            arguments << "-re";                   // リアルタイム再生
                            arguments << "-fflags" << "+genpts";  // PTSを生成
                            arguments << "-analyzeduration" << "500000";  // 分析時間短縮
                            arguments << "-probesize" << "500000";        // プローブサイズ短縮
                            arguments << "-sync" << "ext";        // 外部同期
                            arguments << "-i" << filePath;        // 入力ファイル指定
                            arguments << "-loop" << "-1";         // 無限ループ（継続的読み取り）
                            
                            QProcess::startDetached(ffplayPath, arguments);
                            addLogMessage("✅ FFplay起動完了 - 外部ウィンドウで再生中");
                        } else {
                            addLogMessage("❌ FFplayまたはTSファイルが見つかりません");
                        }
                    });
                }
                
                if (packetCount % 1000 == 0) {
                    addLogMessage(QString("📺 TSデータ受信: パケット#%1, 累計%2KB")
                                 .arg(packetCount).arg(totalBytes/1024));
                }
            });
    addLogMessage("安定TSファイル再生システム設定完了");
#endif
    
    addLogMessage("Step 5: COMPLETED");
    addLogMessage("=== setupDirectStreamPlayer 完了 ===");
}

void MainWindow::onStartDirectStreamClicked()
{
    addLogMessage("=== MainWindow::onStartDirectStreamClicked() START ===");
    
    if (!m_network->isConnected()) {
        addLogMessage("❌ 直接ストリーミング開始失敗: サーバーに接続していません");
        addLogMessage("=== MainWindow::onStartDirectStreamClicked() END (early return) ===");
        return;
    }
    
    addLogMessage("✅ サーバー接続確認完了");
    addLogMessage("=== 直接ストリーミング再生開始 ===");
    
    addLogMessage("Step 1: 動画表示設定");
    // 動画表示設定
#ifdef USE_FFMPEG
    // FFmpegが有効な場合はFFmpegVideoWidgetを使用
    m_ffmpegVideoWidget->setVisible(true);
    m_videoWidget->setVisible(false);
    m_directVideoWidget->setVisible(false);
    addLogMessage("Step 1a: FFmpeg video widget enabled for TS decoding");
#else
    // FFmpegが無い場合はDirectStreamPlayerのVideoWidgetを使用
    m_videoWidget->setVisible(false);
    m_directVideoWidget->setVisible(true);  // DirectStreamPlayer VideoWidget を表示
    addLogMessage("Step 1a: DirectStreamPlayer video widget enabled (no FFmpeg)");
#endif
    addLogMessage("Step 1: COMPLETED");
    
    addLogMessage("Step 2: 動画表示準備完了");
    
    addLogMessage("Step 3: BonDriverNetwork直接ストリーミングモード有効化");
    // BonDriverNetworkの直接ストリーミングモードを有効化
    m_network->setDirectStreamMode(true);
    addLogMessage("Step 3: COMPLETED - direct stream mode enabled");
    
    addLogMessage("Step 4: DirectStreamPlayerで再生開始");
    // DirectStreamPlayerで再生開始
    m_directStreamPlayer->startStreaming();
    addLogMessage("Step 4: COMPLETED - startStreaming() called");
    
    addLogMessage("Step 5: TSストリーム受信状態チェック");
    // 既にTSストリーム受信中でない場合は開始
    if (m_streamStatus->text() == "停止中") {
        addLogMessage("Step 5a: TSストリーム停止中 - 自動開始");
        m_network->startReceiving();
        addLogMessage("Step 5a: COMPLETED - startReceiving() called");
    } else {
        addLogMessage(QString("Step 5a: TSストリーム既に動作中 (%1)").arg(m_streamStatus->text()));
    }
    addLogMessage("Step 5: COMPLETED");
    
    addLogMessage("Step 6: UI状態更新");
    m_directStreamStatus->setText("直接再生中");
    m_directStreamStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
    addLogMessage("Step 6: COMPLETED - UI status updated");
    
    addLogMessage("✅ 直接ストリーミング再生開始 (元TVTest方式)");
    addLogMessage("=== MainWindow::onStartDirectStreamClicked() END ===");
}

void MainWindow::onStopDirectStreamClicked()
{
    addLogMessage("=== 直接ストリーミング再生停止 ===");
    addLogMessage("⚠️ STOP CALLED - 停止が呼ばれました（原因調査用）");
    
    // DirectStreamPlayerで再生停止
    m_directStreamPlayer->stopStreaming();
    
    // BonDriverNetworkの直接ストリーミングモードを無効化
    m_network->setDirectStreamMode(false);
    
    // 直接ストリーミング表示を隠す
    m_directVideoWidget->setVisible(false);
    
    // 元の動画表示に戻す
#ifdef USE_FFMPEG
    m_ffmpegVideoWidget->setVisible(true);
#endif
    
    m_directStreamStatus->setText("停止中");
    m_directStreamStatus->setStyleSheet("color: black; font-weight: bold;");
    m_bufferStatus->setText("バッファ: 0 bytes");
    
    addLogMessage("✅ 直接ストリーミング再生停止");
}

void MainWindow::onDirectStreamPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    QString stateText;
    QString styleSheet;
    
    switch (state) {
    case QMediaPlayer::StoppedState:
        stateText = "停止中";
        styleSheet = "color: black; font-weight: bold;";
        break;
    case QMediaPlayer::PlayingState:
        stateText = "再生中";
        styleSheet = "color: #4CAF50; font-weight: bold;";
        break;
    case QMediaPlayer::PausedState:
        stateText = "一時停止";
        styleSheet = "color: #FF9800; font-weight: bold;";
        break;
    }
    
    m_directStreamStatus->setText(stateText);
    m_directStreamStatus->setStyleSheet(styleSheet);
    
    addLogMessage(QString("直接ストリーミング状態変更: %1").arg(stateText));
}

void MainWindow::onDirectStreamMediaInfoChanged(const QString &info)
{
    // メディア情報をログに出力（詳細情報は必要に応じて）
    static int infoCount = 0;
    infoCount++;
    
    if (infoCount <= 3 || infoCount % 10 == 0) {
        addLogMessage(QString("直接ストリーミング情報: %1").arg(info));
    }
}

void MainWindow::onDirectStreamErrorOccurred(const QString &error)
{
    addLogMessage(QString("⚠️ 直接ストリーミングエラー（無視）: %1").arg(error));
    addLogMessage("DEBUG: エラーハンドラが呼ばれましたが、自動停止はしません");
    
    // Qt Multimediaエラーは無視して直接ストリーミングを継続
    // onStopDirectStreamClicked();  // 自動停止を無効化
}

void MainWindow::onDirectStreamBufferStatusChanged(qint64 bufferSize, int bufferStatus)
{
    // バッファ状態をUIに反映
    QString bufferText = QString("バッファ: %1 KB (%2%)")
                        .arg(bufferSize / 1024)
                        .arg(bufferStatus);
    
    m_bufferStatus->setText(bufferText);
    
    // バッファ状況に応じて色を変更
    if (bufferStatus >= 80) {
        m_bufferStatus->setStyleSheet("color: #4CAF50; font-weight: bold;"); // 緑：十分
    } else if (bufferStatus >= 50) {
        m_bufferStatus->setStyleSheet("color: #FF9800; font-weight: bold;"); // オレンジ：普通
    } else {
        m_bufferStatus->setStyleSheet("color: #f44336; font-weight: bold;"); // 赤：不足
    }
    
    // 定期的なログ出力（スパム防止）
    static int bufferLogCount = 0;
    bufferLogCount++;
    
    if (bufferLogCount <= 5 || bufferLogCount % 20 == 0) {
        addLogMessage(QString("バッファ状況: %1").arg(bufferText));
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    // BonDriver設定
    settings.setValue("bondriver/selectedDriver", m_bonDriverCombo->currentText());
    settings.setValue("bondriver/selectedIndex", m_bonDriverCombo->currentIndex());
    
    // チャンネル設定
    settings.setValue("channel/selectedQuickChannel", m_quickChannelCombo->currentText());
    settings.setValue("channel/selectedQuickIndex", m_quickChannelCombo->currentIndex());
    settings.setValue("channel/space", m_spaceCombo->currentIndex());
    settings.setValue("channel/channel", m_channelSpin->value());
    
    // 接続設定
    settings.setValue("connection/serverHost", "baruma.f5.si");  // 将来の拡張用
    settings.setValue("connection/serverPort", 1192);
    
    addLogMessage("設定を保存しました");
}

void MainWindow::restoreSettings()
{
    QSettings settings;
    
    // BonDriver設定復元
    QString savedBonDriver = settings.value("bondriver/selectedDriver", "").toString();
    int savedBonDriverIndex = settings.value("bondriver/selectedIndex", -1).toInt();
    
    if (!savedBonDriver.isEmpty() && savedBonDriverIndex >= 0 && savedBonDriverIndex < m_bonDriverCombo->count()) {
        m_bonDriverCombo->setCurrentIndex(savedBonDriverIndex);
        addLogMessage(QString("BonDriver復元: %1").arg(savedBonDriver));
    }
    
    // チャンネル設定復元
    QString savedQuickChannel = settings.value("channel/selectedQuickChannel", "").toString();
    int savedQuickChannelIndex = settings.value("channel/selectedQuickIndex", -1).toInt();
    int savedSpace = settings.value("channel/space", 0).toInt();
    int savedChannelNum = settings.value("channel/channel", 1).toInt();
    
    if (!savedQuickChannel.isEmpty() && savedQuickChannelIndex >= 0 && savedQuickChannelIndex < m_quickChannelCombo->count()) {
        m_quickChannelCombo->setCurrentIndex(savedQuickChannelIndex);
        addLogMessage(QString("クイックチャンネル復元: %1").arg(savedQuickChannel));
    }
    
    if (savedSpace >= 0 && savedSpace < m_spaceCombo->count()) {
        m_spaceCombo->setCurrentIndex(savedSpace);
    }
    m_channelSpin->setValue(savedChannelNum);
    
    addLogMessage("前回の設定を復元しました");
}