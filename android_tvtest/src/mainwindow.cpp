#include "mainwindow.h"
#include "core/CoreEngine.h"
#include "network/NetworkManager.h"
#include "utils/Logger.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QMessageBox>
#include <QtCore/QSettings>
#include <QtMultimedia/QAudioOutput>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_videoWidget(nullptr)
    , m_channelListWidget(nullptr)
    , m_playButton(nullptr)
    , m_stopButton(nullptr)
    , m_channelButton(nullptr)
    , m_epgButton(nullptr)
    , m_settingsButton(nullptr)
    , m_statusLabel(nullptr)
    , m_channelLabel(nullptr)
    , m_statusTimer(nullptr)
    , m_mediaPlayer(nullptr)
    , m_coreEngine(nullptr)
    , m_networkManager(nullptr)
    , m_isPlaying(false)
{
    Logger::info("MainWindow: Initializing...");
    
    // Initialize core components
    m_coreEngine = new CoreEngine(this);
    m_networkManager = new NetworkManager(this);
    m_mediaPlayer = new QMediaPlayer(this);
    
    // Setup UI
    setupUI();
    setupMenuBar();
    setupStatusBar();
    connectSignals();
    
    // Setup timer for status updates
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updateStatus);
    m_statusTimer->start(1000); // Update every second
    
    // Window properties
    setWindowTitle("TVTest Android");
    setMinimumSize(800, 600);
    resize(1024, 768);
    
    Logger::info("MainWindow: Initialization complete");
}

MainWindow::~MainWindow()
{
    Logger::info("MainWindow: Shutting down...");
    
    if (m_isPlaying) {
        stopPlayback();
    }
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Video widget (main viewing area)
    m_videoWidget = new QVideoWidget;
    m_videoWidget->setMinimumSize(640, 480);
    m_videoWidget->setStyleSheet("background-color: black;");
    m_mediaPlayer->setVideoOutput(m_videoWidget);
    
    // Channel list
    m_channelListWidget = new QListWidget;
    m_channelListWidget->setMaximumWidth(250);
    m_channelListWidget->addItem("NHK総合");
    m_channelListWidget->addItem("NHK教育");
    m_channelListWidget->addItem("日本テレビ");
    m_channelListWidget->addItem("TBS");
    m_channelListWidget->addItem("フジテレビ");
    m_channelListWidget->addItem("テレビ朝日");
    m_channelListWidget->addItem("テレビ東京");
    
    // Add to splitter
    m_mainSplitter->addWidget(m_videoWidget);
    m_mainSplitter->addWidget(m_channelListWidget);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    // Control buttons layout
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    
    m_playButton = new QPushButton("再生");
    m_stopButton = new QPushButton("停止");
    m_channelButton = new QPushButton("チャンネル");
    m_epgButton = new QPushButton("EPG");
    m_settingsButton = new QPushButton("設定");
    
    m_playButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    
    buttonLayout->addWidget(m_playButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_channelButton);
    buttonLayout->addWidget(m_epgButton);
    buttonLayout->addWidget(m_settingsButton);
    
    mainLayout->addWidget(m_mainSplitter);
    mainLayout->addLayout(buttonLayout);
    
    m_centralWidget->setLayout(mainLayout);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("ファイル");
    QAction *exitAction = fileMenu->addAction("終了");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    QMenu *viewMenu = menuBar()->addMenu("表示");
    QAction *channelListAction = viewMenu->addAction("チャンネルリスト");
    QAction *epgAction = viewMenu->addAction("EPG");
    connect(channelListAction, &QAction::triggered, this, &MainWindow::showChannelList);
    connect(epgAction, &QAction::triggered, this, &MainWindow::showEPG);
    
    QMenu *toolsMenu = menuBar()->addMenu("ツール");
    QAction *settingsAction = toolsMenu->addAction("設定");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("準備完了");
    m_channelLabel = new QLabel("チャンネル: なし");
    
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_channelLabel);
}

void MainWindow::connectSignals()
{
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::playChannel);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopPlayback);
    connect(m_channelButton, &QPushButton::clicked, this, &MainWindow::showChannelList);
    connect(m_epgButton, &QPushButton::clicked, this, &MainWindow::showEPG);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
    
    connect(m_channelListWidget, &QListWidget::itemClicked, this, &MainWindow::onChannelSelected);
}

void MainWindow::playChannel()
{
    if (!m_isPlaying) {
        Logger::info("MainWindow: Starting playback...");
        
        // TODO: Implement actual channel playback via CoreEngine
        QString channelName = m_channelListWidget->currentItem() ? 
                             m_channelListWidget->currentItem()->text() : "NHK総合";
        
        m_currentChannel = channelName;
        m_isPlaying = true;
        
        m_playButton->setEnabled(false);
        m_stopButton->setEnabled(true);
        
        m_statusLabel->setText("再生中");
        m_channelLabel->setText("チャンネル: " + channelName);
        
        Logger::info("MainWindow: Playback started for channel: " + channelName);
    }
}

void MainWindow::stopPlayback()
{
    if (m_isPlaying) {
        Logger::info("MainWindow: Stopping playback...");
        
        m_mediaPlayer->stop();
        m_isPlaying = false;
        
        m_playButton->setEnabled(true);
        m_stopButton->setEnabled(false);
        
        m_statusLabel->setText("停止");
        m_channelLabel->setText("チャンネル: なし");
        
        Logger::info("MainWindow: Playback stopped");
    }
}

void MainWindow::showChannelList()
{
    Logger::info("MainWindow: Showing channel list");
    if (m_channelListWidget->isVisible()) {
        m_channelListWidget->hide();
    } else {
        m_channelListWidget->show();
    }
}

void MainWindow::showEPG()
{
    Logger::info("MainWindow: Showing EPG");
    QMessageBox::information(this, "EPG", "EPG機能は実装予定です");
}

void MainWindow::showSettings()
{
    Logger::info("MainWindow: Showing settings");
    QMessageBox::information(this, "設定", "設定機能は実装予定です");
}

void MainWindow::onChannelSelected()
{
    if (QListWidgetItem *item = m_channelListWidget->currentItem()) {
        Logger::info("MainWindow: Channel selected: " + item->text());
        if (m_isPlaying) {
            m_currentChannel = item->text();
            m_channelLabel->setText("チャンネル: " + item->text());
        }
    }
}

void MainWindow::updateStatus()
{
    // Update status information
    if (m_isPlaying) {
        // TODO: Get actual status from CoreEngine
    }
}