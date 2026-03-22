#include "VLCStreamingPlayer.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <cstring>

VLCStreamingPlayer::VLCStreamingPlayer(QWidget *parent)
    : QObject(parent)
    , m_vlcInstance(nullptr)
    , m_vlcMedia(nullptr)
    , m_vlcPlayer(nullptr)
    , m_bonDriver(std::make_unique<BonDriverNetwork>(this))
    , m_videoWidget(nullptr)
    , m_playerState(Stopped)
    , m_streamingServer(std::make_unique<TSStreamingServer>(this))
    , m_totalBytesReceived(0)
    , m_streamStartTime(0)
    , m_statsTimer(new QTimer(this))
    , m_isVLCInitialized(false)
    , m_volume(50)
    , m_isMuted(false)
{
    // BonDriver信号接続
    connect(m_bonDriver.get(), &BonDriverNetwork::connected,
            this, &VLCStreamingPlayer::onBonDriverConnected);
    connect(m_bonDriver.get(), &BonDriverNetwork::disconnected,
            this, &VLCStreamingPlayer::onBonDriverDisconnected);
    connect(m_bonDriver.get(), &BonDriverNetwork::tsDataReceived,
            this, &VLCStreamingPlayer::onTsDataReceived);
    connect(m_bonDriver.get(), &BonDriverNetwork::errorOccurred,
            this, &VLCStreamingPlayer::onBonDriverError);
    connect(m_bonDriver.get(), &BonDriverNetwork::signalLevelChanged,
            this, &VLCStreamingPlayer::signalLevelChanged);

    // 統計タイマー設定
    m_statsTimer->setInterval(1000); // 1秒間隔
    connect(m_statsTimer, &QTimer::timeout, [this]() {
        if (m_totalBytesReceived > 0 && m_streamStartTime > 0) {
            qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_streamStartTime;
            float mbps = (m_totalBytesReceived * 8.0f / 1000000.0f) / (elapsed / 1000.0f);
            LOG_INFO(QString("ストリーミング統計: %1 MB受信, %2 Mbps")
                    .arg(m_totalBytesReceived / 1048576.0, 0, 'f', 2)
                    .arg(mbps, 0, 'f', 2));
        }
    });

    LOG_INFO("VLCStreamingPlayer初期化完了 - HTTPストリーミングサーバー準備完了");
}

VLCStreamingPlayer::~VLCStreamingPlayer()
{
    stopStreaming();
    releaseVLCResources();
}

bool VLCStreamingPlayer::initializeVLC()
{
    if (m_isVLCInitialized) {
        return true;
    }

    LOG_INFO("=== VLC初期化開始 ===");

    // VLCプラグインパス設定（実行ディレクトリのpluginsフォルダを使用）
    QString appDir = QApplication::applicationDirPath();
    QString pluginsPath = appDir + "/plugins";
    
    LOG_INFO(QString("アプリケーションディレクトリ: %1").arg(appDir));
    LOG_INFO(QString("プラグインパス: %1").arg(pluginsPath));

    // プラグインディレクトリの存在確認
    QDir pluginsDir(pluginsPath);
    if (!pluginsDir.exists()) {
        LOG_CRITICAL(QString("VLCプラグインディレクトリが見つかりません: %1").arg(pluginsPath));
        emit errorOccurred(QString("VLCプラグインディレクトリが見つかりません: %1").arg(pluginsPath));
        return false;
    }

    // VLCインスタンス作成（ライブストリーミング低遅延最適化）
    const char* vlc_args[] = {
        "--intf", "dummy",                           // インターフェース無効
        "--no-video-title-show",                     // タイトル表示無効
        "--live-caching=300",                        // ライブキャッシュ300ms（低遅延）
        "--file-caching=300",                        // ファイルキャッシュ300ms
        "--network-caching=300",                     // ネットワークキャッシュ300ms
        "--clock-jitter=0",                          // クロックジッター無効
        "--clock-synchro=0",                         // クロック同期無効（低遅延）
        "--no-skip-frames",                          // フレームスキップ無効
        "--vout=direct3d9",                          // Direct3D9直接レンダリング（drawableのquery3エラー回避）
        "--verbose=1"                                // 最小ログ
    };

    LOG_INFO(QString("VLC引数数: %1").arg(sizeof(vlc_args) / sizeof(vlc_args[0])));
    for (int i = 0; i < sizeof(vlc_args) / sizeof(vlc_args[0]); i++) {
        LOG_INFO(QString("VLC引数[%1]: %2").arg(i).arg(vlc_args[i]));
    }

    m_vlcInstance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    
    if (!m_vlcInstance) {
        LOG_CRITICAL("VLCインスタンス作成失敗");
        emit errorOccurred("VLCライブラリの初期化に失敗しました");
        return false;
    }

    // VLCメディアプレイヤー作成
    m_vlcPlayer = libvlc_media_player_new(m_vlcInstance);
    if (!m_vlcPlayer) {
        LOG_CRITICAL("VLCメディアプレイヤー作成失敗");
        libvlc_release(m_vlcInstance);
        m_vlcInstance = nullptr;
        emit errorOccurred("VLCメディアプレイヤーの作成に失敗しました");
        return false;
    }

    // VLCイベントコールバック設定
    setupVLCCallbacks();

    // 初期音量設定
    libvlc_audio_set_volume(m_vlcPlayer, m_volume);

    m_isVLCInitialized = true;
    LOG_INFO("VLC初期化完了");
    return true;
}

void VLCStreamingPlayer::setVideoWidget(QWidget *widget)
{
    m_videoWidget = widget;
    
    if (m_vlcPlayer && widget) {
        // Windows環境でのHWND設定
#ifdef _WIN32
        libvlc_media_player_set_hwnd(m_vlcPlayer, (void*)widget->winId());
#else
        libvlc_media_player_set_xwindow(m_vlcPlayer, widget->winId());
#endif
        LOG_INFO("ビデオウィジェット設定完了");
    }
}

bool VLCStreamingPlayer::startStreaming(const QString &host, int port)
{
    if (!initializeVLC()) {
        return false;
    }

    LOG_INFO("=== BonDriverストリーミング開始 ===");
    setState(Buffering);

    // BonDriver接続
    if (!m_bonDriver->connectToServer(host, port)) {
        setState(Error);
        emit errorOccurred("BonDriverサーバーへの接続に失敗しました");
        return false;
    }

    // BonDriver初期化 (PT-S: BS/CS110用)
    if (!m_bonDriver->selectBonDriver("PT-S")) {
        setState(Error);
        emit errorOccurred("BonDriver選択に失敗しました");
        return false;
    }

    // デフォルトチャンネル設定 (BS日テレ: BS=1, Ch=14)
    if (!m_bonDriver->setChannel(BonDriverNetwork::BS, 14)) {
        LOG_WARNING("デフォルトチャンネル設定に失敗、手動設定が必要です");
    }

    // 【修正】TSストリーム受信を3秒後に延期（チャンネル切り替え完了待ち）
    QTimer::singleShot(3000, [this]() {
        LOG_INFO("チャンネル切り替え安定化完了 - TSデータ受信開始");
        m_bonDriver->startReceiving();
        
        // VLCストリーミングを更に2秒後に開始（PAT/PMT受信確保）
        QTimer::singleShot(2000, [this]() {
            if (m_streamingServer->isClientConnected() || m_totalBytesReceived > 0) {
                createStreamingMedia();
                LOG_INFO("PAT/PMT確保後にVLCストリーミング開始");
            } else {
                LOG_WARNING("TSデータ受信できていません、VLC開始を再延期");
                // 再試行ロジック
                QTimer::singleShot(1000, [this]() {
                    if (m_totalBytesReceived > 0) {
                        createStreamingMedia();
                    }
                });
            }
        });
    });

    // ストリーミング統計初期化
    m_totalBytesReceived = 0;
    m_streamStartTime = QDateTime::currentMSecsSinceEpoch();
    m_statsTimer->start();

    LOG_INFO("BonDriverストリーミング開始完了、チャンネル安定化待機中...");
    return true;
}

void VLCStreamingPlayer::stopStreaming()
{
    LOG_INFO("=== ストリーミング停止 ===");

    m_statsTimer->stop();

    // VLCプレイヤー停止
    if (m_vlcPlayer && libvlc_media_player_is_playing(m_vlcPlayer)) {
        libvlc_media_player_stop(m_vlcPlayer);
    }

    // BonDriver切断
    if (m_bonDriver) {
        m_bonDriver->stopReceiving();
        m_bonDriver->disconnectFromServer();
    }

    // HTTPストリーミングサーバー停止
    m_streamingServer->stopServer();

    setState(Stopped);
    emit streamingStopped();
}

bool VLCStreamingPlayer::setChannel(BonDriverNetwork::TuningSpace space, uint32_t channel)
{
    if (!m_isVLCInitialized) {
        LOG_DEBUG("VLC未初期化のためチャンネル設定をスキップ");
        return false;
    }

    if (!m_bonDriver->isConnected()) {
        LOG_DEBUG("BonDriver未接続のためチャンネル設定をスキップ");
        return false;
    }

    LOG_INFO(QString("チャンネル変更: スペース=%1, チャンネル=%2").arg(space).arg(channel));

    // チャンネル変更時は一時停止
    if (m_vlcPlayer && libvlc_media_player_is_playing(m_vlcPlayer)) {
        libvlc_media_player_pause(m_vlcPlayer);
    }

    // BonDriverチャンネル設定
    bool result = m_bonDriver->setChannel(space, channel);
    if (result) {
        // TSストリーム受信開始
        m_bonDriver->startReceiving();
        setState(Buffering);
    }

    return result;
}

void VLCStreamingPlayer::setupVLCCallbacks()
{
    if (!m_vlcPlayer) return;

    libvlc_event_manager_t *eventManager = libvlc_media_player_event_manager(m_vlcPlayer);
    
    // プレイヤーイベント登録
    libvlc_event_attach(eventManager, libvlc_MediaPlayerPlaying, vlcEventCallback, this);
    libvlc_event_attach(eventManager, libvlc_MediaPlayerPaused, vlcEventCallback, this);
    libvlc_event_attach(eventManager, libvlc_MediaPlayerStopped, vlcEventCallback, this);
    libvlc_event_attach(eventManager, libvlc_MediaPlayerEncounteredError, vlcEventCallback, this);
    libvlc_event_attach(eventManager, libvlc_MediaPlayerBuffering, vlcEventCallback, this);
}

void VLCStreamingPlayer::vlcEventCallback(const libvlc_event_t *event, void *userData)
{
    VLCStreamingPlayer *player = static_cast<VLCStreamingPlayer*>(userData);
    
    // 【重要】すべてのVLCイベントをログ出力
    LOG_INFO(QString("【VLCイベント】type=%1, userData=0x%2")
             .arg(event->type)
             .arg(reinterpret_cast<uintptr_t>(userData), 0, 16));
    
    switch (event->type) {
    case libvlc_MediaPlayerPlaying:
        LOG_INFO("VLC: 再生開始");
        player->setState(Playing);
        // 映像設定をメインスレッドで適用（VLC callback thread からは直接呼ばない）
        QMetaObject::invokeMethod(player, "applyVideoSettings", Qt::QueuedConnection);
        emit player->streamingStarted();
        break;
        
    case libvlc_MediaPlayerPaused:
        LOG_INFO("VLC: 一時停止");
        player->setState(Paused);
        break;
        
    case libvlc_MediaPlayerStopped:
        LOG_INFO("VLC: 停止");
        player->setState(Stopped);
        break;
        
    case libvlc_MediaPlayerEncounteredError:
        {
            LOG_CRITICAL("VLC: エラー発生 - 詳細調査が必要");
            // VLCの最後のエラーメッセージを取得
            const char* vlcError = libvlc_errmsg();
            if (vlcError) {
                LOG_CRITICAL(QString("VLCエラー詳細: %1").arg(vlcError));
                libvlc_clearerr();  // エラーメッセージクリア
            }
            player->setState(Error);
            emit player->errorOccurred("VLCプレイヤーでエラーが発生しました");
        }
        break;
        
    case libvlc_MediaPlayerBuffering:
        {
            float percent = event->u.media_player_buffering.new_cache;
            LOG_DEBUG(QString("VLC: バッファリング %1%").arg(percent));
            emit player->bufferingProgress(percent);
            if (percent >= 100.0f) {
                player->setState(Playing);
            }
        }
        break;
        
    default:
        break;
    }
}

void VLCStreamingPlayer::applyVideoSettings()
{
    if (!m_vlcPlayer) return;

    // デコードされた映像サイズを確認（診断用）
    unsigned int videoW = 0, videoH = 0;
    libvlc_video_get_size(m_vlcPlayer, 0, &videoW, &videoH);
    LOG_INFO(QString("🎬 VLCデコード映像サイズ: %1 x %2").arg(videoW).arg(videoH));
    LOG_INFO(QString("🖥️ 映像ウィジェットサイズ: %1 x %2")
             .arg(m_videoWidget ? m_videoWidget->width() : -1)
             .arg(m_videoWidget ? m_videoWidget->height() : -1));

    // デインターレース: Playing イベント後に確実に適用
    // yadif2x = フィールドごとにフレーム生成（60fps出力、1080i対応）
    libvlc_video_set_deinterlace(m_vlcPlayer, "yadif2x");
    LOG_INFO("✅ デインターレース設定: yadif2x");

    // スケール: 0 = ウィジェットに合わせて自動スケール
    libvlc_video_set_scale(m_vlcPlayer, 0);
    LOG_INFO("✅ スケール設定: 自動（ウィジェット全体に表示）");
}

void VLCStreamingPlayer::onBonDriverConnected()
{
    LOG_INFO("BonDriver接続完了");
}

void VLCStreamingPlayer::onBonDriverDisconnected()
{
    LOG_INFO("BonDriver切断");
    setState(Stopped);
}

void VLCStreamingPlayer::onTsDataReceived(const QByteArray &data)
{
    // ストリーミング統計更新
    m_totalBytesReceived += data.size();

    // 【簡素化】HTTPストリーミングサーバーにTSデータを渡すだけ
    m_streamingServer->addTSData(data);
}

void VLCStreamingPlayer::onBonDriverError(const QString &error)
{
    LOG_CRITICAL(QString("BonDriverエラー: %1").arg(error));
    setState(Error);
    emit errorOccurred(error);
}

void VLCStreamingPlayer::processTSStream(const QByteArray &tsData)
{
    static int tsProcessCount = 0;
    tsProcessCount++;
    
    // HTTPストリーミングサーバーにTSデータを追加
    m_streamingServer->addTSData(tsData);
    
    // デバッグログ（最初の10個、その後は1000個ごと）
    if (tsProcessCount <= 10 || tsProcessCount % 1000 == 0) {
        LOG_INFO(QString("【HTTP Streaming #%1】TSデータ: %2 bytes, 合計受信: %3 KB")
                 .arg(tsProcessCount)
                 .arg(tsData.size())
                 .arg(m_totalBytesReceived / 1024));
    }
}

bool VLCStreamingPlayer::createStreamingMedia()
{
    if (!m_vlcInstance || !m_vlcPlayer) {
        LOG_CRITICAL("VLCインスタンス未初期化");
        return false;
    }

    // HTTPストリーミングサーバー開始
    if (!m_streamingServer->startServer(8080)) {
        LOG_CRITICAL("TSストリーミングサーバー開始失敗");
        return false;
    }

    QString streamUrl = m_streamingServer->getStreamUrl();
    LOG_INFO(QString("🌐 HTTPストリーミング作成: %1").arg(streamUrl));

    // 【革命的簡素化】VLCにHTTP URLを直接渡すだけ
    m_vlcMedia = libvlc_media_new_location(m_vlcInstance, streamUrl.toUtf8().data());

    // MPEG-2 TS + ライブストリーミング最適化
    if (m_vlcMedia) {
        libvlc_media_add_option(m_vlcMedia, ":demux=ts");         // TSデマルチプレクサ強制
        libvlc_media_add_option(m_vlcMedia, ":ts-es-id-pid");     // ES ID設定
        libvlc_media_add_option(m_vlcMedia, ":no-ts-trust-pcr");  // PCR信頼無効
        
        // ライブストリーミング専用最適化
        libvlc_media_add_option(m_vlcMedia, ":ts-seek-percent=false");  // シーク無効
        libvlc_media_add_option(m_vlcMedia, ":live-caching=300");         // 300ms低遅延キャッシング
        libvlc_media_add_option(m_vlcMedia, ":network-caching=300");      // ネットワークキャッシング300ms
        libvlc_media_add_option(m_vlcMedia, ":input-repeat=999999");      // 無限ループ
        libvlc_media_add_option(m_vlcMedia, ":start-time=0");                     // 開始時刻指定
    }

    if (!m_vlcMedia) {
        LOG_CRITICAL("VLC公式コールバックメディア作成失敗");
        return false;
    }

    LOG_INFO("VLC公式コールバックメディア作成成功");

    // メディアをプレイヤーに設定
    libvlc_media_player_set_media(m_vlcPlayer, m_vlcMedia);
    
    // 再生開始
    int result = libvlc_media_player_play(m_vlcPlayer);
    if (result == 0) {
        LOG_INFO("VLC公式コールバックストリーミング開始成功");

        setState(Playing);
        return true;
    } else {
        LOG_CRITICAL(QString("VLC公式コールバックストリーミング開始失敗: %1").arg(result));
        return false;
    }
}

// 【削除】VLCコールバック関数群 - HTTPストリーミング使用により完全不要

void VLCStreamingPlayer::setState(PlayerState state)
{
    if (m_playerState != state) {
        m_playerState = state;
        emit stateChanged(state);
        
        const char* stateNames[] = {"Stopped", "Playing", "Paused", "Buffering", "Error"};
        LOG_DEBUG(QString("プレイヤー状態変更: %1").arg(stateNames[state]));
    }
}

float VLCStreamingPlayer::getSignalLevel() const
{
    return m_bonDriver ? m_bonDriver->getSignalLevel() : 0.0f;
}

void VLCStreamingPlayer::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);
    if (m_vlcPlayer) {
        libvlc_audio_set_volume(m_vlcPlayer, m_volume);
    }
}

int VLCStreamingPlayer::getVolume() const
{
    return m_vlcPlayer ? libvlc_audio_get_volume(m_vlcPlayer) : m_volume;
}

void VLCStreamingPlayer::setMute(bool muted)
{
    m_isMuted = muted;
    if (m_vlcPlayer) {
        libvlc_audio_set_mute(m_vlcPlayer, muted ? 1 : 0);
    }
}

bool VLCStreamingPlayer::isMuted() const
{
    return m_vlcPlayer ? libvlc_audio_get_mute(m_vlcPlayer) == 1 : m_isMuted;
}

void VLCStreamingPlayer::releaseVLCResources()
{
    if (m_vlcMedia) {
        libvlc_media_release(m_vlcMedia);
        m_vlcMedia = nullptr;
    }
    
    if (m_vlcPlayer) {
        libvlc_media_player_release(m_vlcPlayer);
        m_vlcPlayer = nullptr;
    }
    
    if (m_vlcInstance) {
        libvlc_release(m_vlcInstance);
        m_vlcInstance = nullptr;
    }
    
    m_isVLCInitialized = false;
    LOG_INFO("VLCリソース解放完了");
}