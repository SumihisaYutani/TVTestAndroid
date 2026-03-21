#ifndef VLCSTREAMINGPLAYER_H
#define VLCSTREAMINGPLAYER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QString>
#include <QDebug>
#include <QThread>
#include <memory>

// VLC includes
#include <vlc/vlc.h>
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>

#include "BonDriverNetwork.h"
#include "TSStreamingServer.h"

/**
 * @brief VLCを使用したリアルタイムTSストリーミングプレイヤー
 * 
 * BonDriverNetworkから受信したTSストリームをVLCで直接再生する
 * メモリコピーを最小限に抑えたリアルタイム実装
 */
class VLCStreamingPlayer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief プレイヤー状態
     */
    enum PlayerState {
        Stopped = 0,
        Playing = 1,
        Paused = 2,
        Buffering = 3,
        Error = 4
    };

    explicit VLCStreamingPlayer(QWidget *parent = nullptr);
    ~VLCStreamingPlayer();

    // プレイヤー制御
    bool initializeVLC();
    void setVideoWidget(QWidget *widget);
    bool startStreaming(const QString &host = "baruma.f5.si", int port = 1192);
    void stopStreaming();
    bool setChannel(BonDriverNetwork::TuningSpace space, uint32_t channel);
    
    // 状態取得
    PlayerState getState() const { return m_playerState; }
    bool isStreaming() const { return m_playerState == Playing || m_playerState == Buffering; }
    float getSignalLevel() const;
    
    // 音量制御
    void setVolume(int volume);
    int getVolume() const;
    void setMute(bool muted);
    bool isMuted() const;

signals:
    void stateChanged(PlayerState state);
    void streamingStarted();
    void streamingStopped();
    void signalLevelChanged(float level);
    void errorOccurred(const QString &error);
    void bufferingProgress(float percentage);

private slots:
    void onBonDriverConnected();
    void onBonDriverDisconnected();
    void onTsDataReceived(const QByteArray &data);
    void onBonDriverError(const QString &error);

private:
    // VLC初期化・解放
    bool setupVLCInstance();
    void releaseVLCResources();
    
    // ストリーミング管理
    bool createStreamingMedia();
    void setupVLCCallbacks();
    static void vlcEventCallback(const libvlc_event_t *event, void *userData);
    
    // TSストリーム処理（ゼロコピー）
    void processTSStream(const QByteArray &tsData);
    
    // 【削除】VLCコールバック関数 - HTTPストリーミング使用により不要
    
    // ステート管理
    void setState(PlayerState state);

private:
    // VLCコンポーネント
    libvlc_instance_t *m_vlcInstance;
    libvlc_media_t *m_vlcMedia;
    libvlc_media_player_t *m_vlcPlayer;
    
    // ストリーミング設定
    std::unique_ptr<BonDriverNetwork> m_bonDriver;
    QWidget *m_videoWidget;
    PlayerState m_playerState;
    
    // HTTPストリーミングサーバー（コールバック不要）
    std::unique_ptr<TSStreamingServer> m_streamingServer;
    
    // パフォーマンス統計
    qint64 m_totalBytesReceived;
    qint64 m_streamStartTime;
    QTimer *m_statsTimer;
    
    // VLC設定
    bool m_isVLCInitialized;
    int m_volume;
    bool m_isMuted;
};

#endif // VLCSTREAMINGPLAYER_H