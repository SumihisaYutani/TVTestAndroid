#include "CoreEngine.h"
#include "../network/NetworkManager.h"
#include "../database/EPGDatabase.h"
#include "../utils/Logger.h"

#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtMultimedia/QAudioOutput>
#include <QtNetwork/QNetworkReply>

CoreEngine::CoreEngine(QObject *parent)
    : QObject(parent)
    , m_state(EngineState::Stopped)
    , m_isPlaying(false)
    , m_mediaPlayer(nullptr)
    , m_networkManager(nullptr)
    , m_networkAccess(nullptr)
    , m_epgDatabase(nullptr)
    , m_statusTimer(nullptr)
    , m_signalLevel(0)
    , m_signalQuality(0.0)
{
    Logger::info("CoreEngine: Initializing...");
    
    // Initialize media player (Qt Multimedia replacement for DirectShow)
    m_mediaPlayer = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(audioOutput);
    
    // Initialize network manager
    m_networkManager = new NetworkManager(this);
    m_networkAccess = new QNetworkAccessManager(this);
    
    // Initialize EPG database
    m_epgDatabase = new EPGDatabase(this);
    
    // Setup status monitoring timer
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &CoreEngine::updateStatus);
    
    // Connect media player signals
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &CoreEngine::onMediaPlayerStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred,
            this, &CoreEngine::onMediaPlayerError);
    
    // Connect network manager signals
    connect(m_networkManager, &NetworkManager::streamReady,
            this, &CoreEngine::onNetworkStreamReady);
    connect(m_networkManager, &NetworkManager::errorOccurred,
            this, &CoreEngine::onNetworkError);
    
    // Initialize default channels
    initializeDefaultChannels();
    
    Logger::info("CoreEngine: Initialization complete");
}

CoreEngine::~CoreEngine()
{
    Logger::info("CoreEngine: Shutting down...");
    shutdown();
}

bool CoreEngine::initialize()
{
    Logger::info("CoreEngine: Starting initialization...");
    
    setState(EngineState::Starting);
    
    // Initialize EPG database
    if (!m_epgDatabase->initialize()) {
        Logger::error("CoreEngine: Failed to initialize EPG database");
        setState(EngineState::Error);
        return false;
    }
    
    // Initialize network manager
    if (!m_networkManager->initialize()) {
        Logger::error("CoreEngine: Failed to initialize network manager");
        setState(EngineState::Error);
        return false;
    }
    
    // Start status monitoring
    m_statusTimer->start(1000);
    
    setState(EngineState::Running);
    Logger::info("CoreEngine: Initialization successful");
    
    return true;
}

void CoreEngine::shutdown()
{
    if (m_state == EngineState::Stopped) {
        return;
    }
    
    Logger::info("CoreEngine: Shutting down...");
    setState(EngineState::Stopping);
    
    // Stop playback
    if (m_isPlaying) {
        stopPlayback();
    }
    
    // Stop status timer
    if (m_statusTimer && m_statusTimer->isActive()) {
        m_statusTimer->stop();
    }
    
    // Shutdown components
    if (m_networkManager) {
        m_networkManager->shutdown();
    }
    
    if (m_epgDatabase) {
        m_epgDatabase->shutdown();
    }
    
    setState(EngineState::Stopped);
    Logger::info("CoreEngine: Shutdown complete");
}

bool CoreEngine::setChannel(const ChannelInfo &channel)
{
    Logger::info("CoreEngine: Setting channel: " + channel.name);
    
    if (m_state != EngineState::Running) {
        Logger::warning("CoreEngine: Cannot set channel - engine not running");
        return false;
    }
    
    // Stop current playback
    if (m_isPlaying) {
        stopPlayback();
    }
    
    m_currentChannel = channel;
    
    // Emit channel changed signal
    emit channelChanged(channel);
    
    // For network channels, prepare stream
    if (channel.type == ChannelType::Network && !channel.streamUrl.isEmpty()) {
        return openStream(channel.streamUrl);
    }
    
    Logger::info("CoreEngine: Channel set successfully");
    return true;
}

bool CoreEngine::setChannelByIndex(int index)
{
    if (index < 0 || index >= m_channelList.size()) {
        Logger::warning("CoreEngine: Invalid channel index: " + QString::number(index));
        return false;
    }
    
    return setChannel(m_channelList[index]);
}

QList<ChannelInfo> CoreEngine::getChannelList() const
{
    return m_channelList;
}

bool CoreEngine::startPlayback()
{
    Logger::info("CoreEngine: Starting playback...");
    
    if (m_state != EngineState::Running) {
        Logger::warning("CoreEngine: Cannot start playback - engine not running");
        return false;
    }
    
    if (m_isPlaying) {
        Logger::info("CoreEngine: Already playing");
        return true;
    }
    
    // Check if we have a valid channel
    if (m_currentChannel.name.isEmpty()) {
        Logger::warning("CoreEngine: No channel selected");
        return false;
    }
    
    // Start media player (replaces DirectShow IMediaControl::Run())
    m_mediaPlayer->play();
    m_isPlaying = true;
    
    Logger::info("CoreEngine: Playback started for channel: " + m_currentChannel.name);
    return true;
}

bool CoreEngine::stopPlayback()
{
    Logger::info("CoreEngine: Stopping playback...");
    
    if (!m_isPlaying) {
        Logger::info("CoreEngine: Already stopped");
        return true;
    }
    
    // Stop media player (replaces DirectShow IMediaControl::Stop())
    m_mediaPlayer->stop();
    m_isPlaying = false;
    
    // Close any open streams
    closeStream();
    
    Logger::info("CoreEngine: Playback stopped");
    return true;
}

bool CoreEngine::openStream(const QString &url)
{
    Logger::info("CoreEngine: Opening stream: " + url);
    
    if (url.isEmpty()) {
        Logger::warning("CoreEngine: Empty stream URL");
        return false;
    }
    
    // Handle different stream types
    if (url.startsWith("http://") || url.startsWith("https://")) {
        // Network stream (HLS, DASH, etc.)
        Logger::info("CoreEngine: Opening network stream");
        return m_networkManager->openNetworkStream(url);
    }
    else if (url.startsWith("udp://")) {
        // UDP multicast stream
        Logger::info("CoreEngine: Opening UDP stream");
        return m_networkManager->openUDPStream(url);
    }
    else if (url.startsWith("file://") || QFile::exists(url)) {
        // Local file
        Logger::info("CoreEngine: Opening local file");
        m_mediaPlayer->setSource(QUrl::fromLocalFile(url));
        return true;
    }
    else {
        Logger::error("CoreEngine: Unsupported stream URL format: " + url);
        return false;
    }
}

void CoreEngine::closeStream()
{
    Logger::info("CoreEngine: Closing stream");
    
    // Stop media player
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        m_mediaPlayer->setSource(QUrl());
    }
    
    // Close network streams
    if (m_networkManager) {
        m_networkManager->closeStream();
    }
}

void CoreEngine::updateEPG()
{
    Logger::info("CoreEngine: Updating EPG data...");
    
    if (!m_epgDatabase) {
        Logger::warning("CoreEngine: EPG database not available");
        return;
    }
    
    // TODO: Implement EPG update logic using LibISDB
    // This would integrate with LibISDB::EPGDatabaseFilter
    
    emit epgUpdated();
    Logger::info("CoreEngine: EPG update complete");
}

bool CoreEngine::isEPGAvailable() const
{
    return m_epgDatabase && m_epgDatabase->isInitialized();
}

void CoreEngine::onNetworkStreamReady(const QString &url)
{
    Logger::info("CoreEngine: Network stream ready: " + url);
    
    // Set the stream as media source
    m_mediaPlayer->setSource(QUrl(url));
    
    // Update signal quality (simulated)
    m_signalLevel = 80;
    m_signalQuality = 0.8;
    emit signalLevelChanged(m_signalLevel);
}

void CoreEngine::onNetworkError(const QString &error)
{
    Logger::error("CoreEngine: Network error: " + error);
    emit errorOccurred("Network error: " + error);
    
    // Reset signal indicators
    m_signalLevel = 0;
    m_signalQuality = 0.0;
    emit signalLevelChanged(m_signalLevel);
}

void CoreEngine::updateStatus()
{
    // Update signal quality and other status information
    // This would integrate with actual tuner hardware via LibISDB
    
    if (m_isPlaying) {
        // Simulate signal monitoring
        if (m_signalLevel > 0) {
            // Add some variation to simulate real signal monitoring
            int variation = (QRandomGenerator::global()->bounded(10)) - 5;
            m_signalLevel = qBound(0, m_signalLevel + variation, 100);
            emit signalLevelChanged(m_signalLevel);
        }
    }
}

void CoreEngine::onMediaPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
    Logger::info("CoreEngine: Media player state changed: " + QString::number(static_cast<int>(state)));
    
    switch (state) {
    case QMediaPlayer::PlayingState:
        m_isPlaying = true;
        break;
    case QMediaPlayer::PausedState:
        // Handle pause if needed
        break;
    case QMediaPlayer::StoppedState:
        m_isPlaying = false;
        break;
    }
}

void CoreEngine::onMediaPlayerError(QMediaPlayer::Error error)
{
    QString errorString = "Media player error: " + QString::number(static_cast<int>(error));
    Logger::error("CoreEngine: " + errorString);
    
    emit errorOccurred(errorString);
    
    // Reset playback state
    m_isPlaying = false;
}

void CoreEngine::setState(EngineState newState)
{
    if (m_state != newState) {
        Logger::info("CoreEngine: State change: " + QString::number(static_cast<int>(m_state)) + 
                    " -> " + QString::number(static_cast<int>(newState)));
        m_state = newState;
        emit stateChanged(newState);
    }
}

void CoreEngine::loadChannelList()
{
    Logger::info("CoreEngine: Loading channel list...");
    
    // TODO: Load from configuration file or database
    // For now, use the default channels
    initializeDefaultChannels();
    
    Logger::info("CoreEngine: Loaded " + QString::number(m_channelList.size()) + " channels");
}

void CoreEngine::initializeDefaultChannels()
{
    m_channelList.clear();
    
    // Add default Japanese terrestrial channels
    ChannelInfo channel;
    
    channel = { 1, "NHK総合", "NHK", 1024, 32736, 32736, ChannelType::Terrestrial, "", true };
    m_channelList.append(channel);
    
    channel = { 2, "NHK教育", "NHK", 1032, 32736, 32736, ChannelType::Terrestrial, "", true };
    m_channelList.append(channel);
    
    channel = { 4, "日本テレビ", "NTV", 1040, 32744, 32744, ChannelType::Terrestrial, "", true };
    m_channelList.append(channel);
    
    channel = { 6, "TBS", "TBS", 1048, 32748, 32748, ChannelType::Terrestrial, "", true };
    m_channelList.append(channel);
    
    channel = { 8, "フジテレビ", "CX", 1056, 32752, 32752, ChannelType::Terrestrial, "", true };
    m_channelList.append(channel);
    
    channel = { 10, "テレビ朝日", "EX", 1064, 32756, 32756, ChannelType::Terrestrial, "", true };
    m_channelList.append(channel);
    
    channel = { 12, "テレビ東京", "TX", 1072, 32760, 32760, ChannelType::Terrestrial, "", true };
    m_channelList.append(channel);
    
    // Add sample network stream channels
    channel = { 100, "Sample HLS Stream", "Network", 0, 0, 0, ChannelType::Network, 
               "https://sample-videos.com/zip/10mp4/mp4/SampleVideo_1280x720_1mb.mp4", true };
    m_channelList.append(channel);
    
    Logger::info("CoreEngine: Initialized " + QString::number(m_channelList.size()) + " default channels");
}