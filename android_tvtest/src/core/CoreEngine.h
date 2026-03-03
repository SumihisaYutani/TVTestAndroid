#ifndef COREENGINE_H
#define COREENGINE_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QRandomGenerator>
#include <QtMultimedia/QMediaPlayer>
#include <QtNetwork/QNetworkAccessManager>

// Forward declarations for LibISDB integration (when ported)
// #include "../../src/LibISDB/LibISDB/LibISDB.hpp"

class NetworkManager;
class EPGDatabase;

enum class EngineState {
    Stopped,
    Starting,
    Running,
    Stopping,
    Error
};

enum class ChannelType {
    Terrestrial,
    BS,
    CS,
    Network
};

struct ChannelInfo {
    int id;
    QString name;
    QString networkName;
    int serviceId;
    int transportStreamId;
    int networkId;
    ChannelType type;
    QString streamUrl; // For network streams
    bool isActive;
};

class CoreEngine : public QObject
{
    Q_OBJECT

public:
    explicit CoreEngine(QObject *parent = nullptr);
    ~CoreEngine();

    // Engine control
    bool initialize();
    void shutdown();
    
    EngineState getState() const { return m_state; }
    
    // Channel management
    bool setChannel(const ChannelInfo &channel);
    bool setChannelByIndex(int index);
    ChannelInfo getCurrentChannel() const { return m_currentChannel; }
    QList<ChannelInfo> getChannelList() const;
    
    // Playback control
    bool startPlayback();
    bool stopPlayback();
    bool isPlaying() const { return m_isPlaying; }
    
    // Stream control (replaces DirectShow functionality)
    bool openStream(const QString &url);
    void closeStream();
    
    // Signal quality (placeholder for tuner info)
    int getSignalLevel() const { return m_signalLevel; }
    double getSignalQuality() const { return m_signalQuality; }
    
    // EPG integration
    void updateEPG();
    bool isEPGAvailable() const;

signals:
    void stateChanged(EngineState newState);
    void channelChanged(const ChannelInfo &channel);
    void signalLevelChanged(int level);
    void errorOccurred(const QString &error);
    void epgUpdated();

public slots:
    void onNetworkStreamReady(const QString &url);
    void onNetworkError(const QString &error);

private slots:
    void updateStatus();
    void onMediaPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaPlayerError(QMediaPlayer::Error error);

private:
    void setState(EngineState newState);
    void loadChannelList();
    void initializeDefaultChannels();
    
    // LibISDB integration methods (to be implemented)
    // bool initializeLibISDB();
    // void shutdownLibISDB();
    // bool setupTSProcessor();
    
    EngineState m_state;
    bool m_isPlaying;
    
    // Current channel
    ChannelInfo m_currentChannel;
    QList<ChannelInfo> m_channelList;
    
    // Media playback (Qt Multimedia replacement for DirectShow)
    QMediaPlayer *m_mediaPlayer;
    
    // Network streaming
    NetworkManager *m_networkManager;
    QNetworkAccessManager *m_networkAccess;
    
    // Database
    EPGDatabase *m_epgDatabase;
    
    // Status monitoring
    QTimer *m_statusTimer;
    int m_signalLevel;
    double m_signalQuality;
    
    // LibISDB components (when ported)
    // LibISDB::ViewerEngine *m_viewerEngine;
    // LibISDB::TSProcessor *m_tsProcessor;
    // LibISDB::EPGDatabaseFilter *m_epgFilter;
};

#endif // COREENGINE_H