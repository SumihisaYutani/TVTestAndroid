#include "DirectStreamPlayer.h"
#include "../utils/Logger.h"
#include <QVideoWidget>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QFile>
#include <QProcess>

DirectStreamPlayer::DirectStreamPlayer(QObject *parent)
    : QObject(parent)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_streamDevice(nullptr)
    , m_videoWidget(nullptr)
    , m_progressTimer(new QTimer(this))
    , m_autoStartTimer(new QTimer(this))
    , m_streamingActive(false)
    , m_lastBufferSize(0)
    , m_totalDataReceived(0)
    , m_autoStartAttempts(0)
    , m_hasStartedPlayback(false)
    , m_ffplayProcess(nullptr)
{
    // Qt Multimedia初期化
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // TSストリームデバイス初期化
    m_streamDevice = new TsStreamDevice(this);
    
    // 🔧 重要: QMediaPlayerにTsStreamDeviceをメディアソースとして設定
    m_mediaPlayer->setSourceDevice(m_streamDevice);
    LOG_INFO("DirectStreamPlayer: TsStreamDevice set as media source");

#ifdef USE_FFMPEG
    // FFmpegDecoder初期化
    m_ffmpegDecoder = new FFmpegDecoder(this);
    m_ffmpegVideoWidget = nullptr; // 後でMainWindowから設定される
    
    // FFmpegDecoderの初期化
    if (m_ffmpegDecoder->initialize()) {
        LOG_INFO("DirectStreamPlayer: FFmpegDecoder initialized successfully");
    } else {
        LOG_WARNING("DirectStreamPlayer: FFmpegDecoder initialization failed");
    }
    
    // FFmpegDecoderのシグナル接続
    connect(m_ffmpegDecoder, &FFmpegDecoder::frameReady, this, [this](const QImage &frame) {
        if (m_ffmpegVideoWidget) {
            m_ffmpegVideoWidget->displayFrame(frame);
        }
    });
#endif

    // タイマー設定
    m_progressTimer->setInterval(1000); // 1秒間隔
    m_progressTimer->setSingleShot(false);

    m_autoStartTimer->setInterval(500); // 0.5秒間隔
    m_autoStartTimer->setSingleShot(false);

    // シグナル接続
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &DirectStreamPlayer::onPlaybackStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &DirectStreamPlayer::onMediaStatusChanged);
    connect(m_mediaPlayer, QOverload<QMediaPlayer::Error, const QString &>::of(&QMediaPlayer::errorOccurred),
            this, &DirectStreamPlayer::onErrorOccurred);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged,
            this, &DirectStreamPlayer::onDurationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged,
            this, &DirectStreamPlayer::onPositionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::bufferProgressChanged,
            this, &DirectStreamPlayer::onBufferProgressChanged);

    connect(m_streamDevice, &TsStreamDevice::dataAvailable,
            this, &DirectStreamPlayer::onStreamDeviceDataAvailable);
    connect(m_streamDevice, &TsStreamDevice::bufferSizeChanged,
            this, &DirectStreamPlayer::onStreamDeviceBufferSizeChanged);

    connect(m_progressTimer, &QTimer::timeout,
            this, &DirectStreamPlayer::checkPlaybackProgress);
    connect(m_autoStartTimer, &QTimer::timeout,
            this, &DirectStreamPlayer::autoStartPlayback);

    LOG_INFO("DirectStreamPlayer: Initialized direct stream player");
}

DirectStreamPlayer::~DirectStreamPlayer()
{
    stopStreaming();
}

void DirectStreamPlayer::setVideoWidget(QVideoWidget *videoWidget)
{
    m_videoWidget = videoWidget;
    if (m_videoWidget) {
        m_mediaPlayer->setVideoOutput(m_videoWidget);
        LOG_INFO("DirectStreamPlayer: Video widget set");
    }
}

void DirectStreamPlayer::addTsStream(const QByteArray &tsData)
{
    if (tsData.isEmpty()) return;

    // TSストリームデバイスに追加
    m_streamDevice->addTsData(tsData);
    m_totalDataReceived += tsData.size();

    // バッファサイズ監視
    qint64 bufferSize = m_streamDevice->bufferSize();
    
    static int logCounter = 0;
    logCounter++;
    
    // 500回ごとにバッファ状態をログ出力（調査用）
    if (logCounter % 500 == 0) {
        LOG_INFO(QString("📊 TSバッファ状態: %1 KB, 総受信: %2 KB, ストリーミング: %3")
                .arg(bufferSize / 1024)
                .arg(m_totalDataReceived / 1024)
                .arg(m_streamingActive ? "ON" : "OFF"));
    }

    // 初回再生開始条件
    if (!m_hasStartedPlayback && bufferSize >= BUFFER_THRESHOLD && m_streamingActive) {
        LOG_INFO(QString("🚀 初回再生開始条件達成: バッファ %1 KB").arg(bufferSize / 1024));
        m_hasStartedPlayback = true;  // ✅ フラグを立てて二重実行を防止
        
        // 🔧 376KBの大容量バッファでFFplay再生
        LOG_INFO("🎬 大容量TSバッファによる外部FFplay再生開始");
        
        // TSファイル保存→外部FFplay起動
        startExternalFFplayPlayback();
        
        LOG_INFO("🎬 大容量FFplay再生開始完了");
    }
}

void DirectStreamPlayer::startStreaming()
{
    LOG_DEBUG("=== DirectStreamPlayer::startStreaming() START ===");
    
    if (m_streamingActive) {
        LOG_WARNING("Streaming already active - returning early");
        return;
    }

    LOG_DEBUG("Step 1: Setting streaming state variables");
    m_streamingActive = true;
    m_hasStartedPlayback = false;
    m_autoStartAttempts = 0;
    m_totalDataReceived = 0;
    LOG_DEBUG("Step 1: COMPLETED - streaming state variables set");

    LOG_DEBUG("Step 2: Opening stream device");
    // ストリームデバイスを開く
    if (!m_streamDevice->isOpen()) {
        LOG_DEBUG("Step 2a: Stream device not open, opening in ReadOnly mode");
        bool openResult = m_streamDevice->open(QIODevice::ReadOnly);
        LOG_DEBUG(QString("Step 2a: Stream device open result: %1").arg(openResult ? "SUCCESS" : "FAILED"));
        if (!openResult) {
            LOG_CRITICAL("Step 2a: FAILED - Cannot open stream device");
            return;
        }
    } else {
        LOG_DEBUG("Step 2a: Stream device already open");
    }
    LOG_DEBUG("Step 2: COMPLETED - stream device ready");

    LOG_DEBUG("Step 3: Configuring stream device");
    m_streamDevice->setStreaming(true);
    LOG_DEBUG("Step 3a: setStreaming(true) completed");
    
    m_streamDevice->clearBuffer();
    LOG_DEBUG("Step 3b: clearBuffer() completed");
    LOG_DEBUG("Step 3: COMPLETED - stream device configured");

    LOG_DEBUG("Step 4: Qt Multimedia setup (TSファイル再生モード)");
    // TSファイル再生は一時的に無効化 - 安定したバッファリング後に実行
    LOG_INFO("📁 TSファイル再生は後で実行 - まずバッファリング完了まで待機");
    LOG_DEBUG("Step 4: COMPLETED - Qt Multimedia setup deferred");

    LOG_DEBUG("Step 5: Starting progress timer");
    // 進行監視開始
    m_progressTimer->start();
    LOG_DEBUG("Step 5: COMPLETED - progress timer started");

    LOG_INFO("Started direct streaming successfully");
    LOG_DEBUG("=== DirectStreamPlayer::startStreaming() END ===");
}

void DirectStreamPlayer::stopStreaming()
{
    if (!m_streamingActive) return;

    m_streamingActive = false;
    m_hasStartedPlayback = false;

    // タイマー停止
    m_progressTimer->stop();
    m_autoStartTimer->stop();

    // 再生停止
    m_mediaPlayer->stop();

    // ストリームデバイス停止
    m_streamDevice->setStreaming(false);
    if (m_streamDevice->isOpen()) {
        m_streamDevice->close();
    }

    emit playbackStateChanged(m_mediaPlayer->playbackState());

    LOG_INFO( 
        QString("Stopped streaming, total data processed: %1 bytes").arg(m_totalDataReceived));
}

void DirectStreamPlayer::pauseStreaming()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
        LOG_INFO( "Paused streaming");
    }
}

void DirectStreamPlayer::resumeStreaming()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PausedState) {
        m_mediaPlayer->play();
        LOG_INFO( "Resumed streaming");
    }
}

QMediaPlayer::PlaybackState DirectStreamPlayer::playbackState() const
{
    return m_mediaPlayer->playbackState();
}

void DirectStreamPlayer::setVolume(float volume)
{
    m_audioOutput->setVolume(qBound(0.0f, volume, 1.0f));
}

void DirectStreamPlayer::setMuted(bool muted)
{
    m_audioOutput->setMuted(muted);
}

qint64 DirectStreamPlayer::bufferSize() const
{
    return m_streamDevice->bufferSize();
}

// Private slots implementation

void DirectStreamPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    const char* stateStr = "";
    switch (state) {
    case QMediaPlayer::StoppedState: stateStr = "Stopped"; break;
    case QMediaPlayer::PlayingState: stateStr = "Playing"; break;
    case QMediaPlayer::PausedState: stateStr = "Paused"; break;
    }

    LOG_INFO(QString("🎭 Playback State: %1").arg(stateStr));

    // 状態変更の詳細分析
    if (state == QMediaPlayer::StoppedState) {
        if (m_hasStartedPlayback) {
            LOG_WARNING("⚠️ 再生が停止しました - 再生継続を試行します");
            // 自動的に再再生を試行
            QTimer::singleShot(100, this, [this]() {
                if (m_streamingActive && m_streamDevice->bufferSize() > 0) {
                    LOG_INFO("🔄 自動再開を試行");
                    m_mediaPlayer->play();
                }
            });
        } else {
            LOG_INFO("ℹ️ 初期停止状態");
        }
    }
    
    if (state == QMediaPlayer::PlayingState) {
        m_hasStartedPlayback = true;
        m_autoStartTimer->stop();
        LOG_INFO("✅ 再生開始成功");
    }
    
    if (state == QMediaPlayer::PausedState) {
        LOG_WARNING("⚠️ 再生が一時停止されました");
    }

    emit playbackStateChanged(state);
}

void DirectStreamPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    const char* statusStr = "";
    switch (status) {
    case QMediaPlayer::NoMedia: statusStr = "NoMedia"; break;
    case QMediaPlayer::LoadingMedia: statusStr = "LoadingMedia"; break;
    case QMediaPlayer::LoadedMedia: statusStr = "LoadedMedia"; break;
    case QMediaPlayer::StalledMedia: statusStr = "StalledMedia"; break;
    case QMediaPlayer::BufferingMedia: statusStr = "BufferingMedia"; break;
    case QMediaPlayer::BufferedMedia: statusStr = "BufferedMedia"; break;
    case QMediaPlayer::EndOfMedia: statusStr = "EndOfMedia"; break;
    case QMediaPlayer::InvalidMedia: statusStr = "InvalidMedia"; break;
    }

    LOG_INFO(QString("🎬 Media Status: %1").arg(statusStr));

    // 問題の状態を特別に処理
    if (status == QMediaPlayer::EndOfMedia) {
        LOG_WARNING("⚠️ EndOfMedia detected - TSストリームが終了と認識されました");
        // 再生を停止せずに継続するため、何もしない
        return;
    }
    
    if (status == QMediaPlayer::InvalidMedia) {
        LOG_CRITICAL("❌ InvalidMedia - Qt MultimediaがTSストリーム形式を認識できません");
        return;
    }
    
    if (status == QMediaPlayer::StalledMedia) {
        LOG_WARNING("⚠️ StalledMedia - データ供給が停滞しています");
        return;
    }

    if (status == QMediaPlayer::BufferedMedia && !m_hasStartedPlayback) {
        LOG_INFO("✅ BufferedMedia - 自動再生開始");
        m_mediaPlayer->play();
    }

    updateMediaInfo();
}

void DirectStreamPlayer::onErrorOccurred(QMediaPlayer::Error error, const QString &errorString)
{
    QString errorMsg = QString("Media player error: %1").arg(errorString);
    LOG_WARNING(QString("⚠️ Qt Multimedia エラー（無視）: %1").arg(errorString));
    // Qt Multimediaエラーは無視して直接ストリーミングを継続
    // emit errorOccurred(errorMsg);  // エラーイベント無効化
}

void DirectStreamPlayer::onDurationChanged(qint64 duration)
{
    Q_UNUSED(duration)
    updateMediaInfo();
}

void DirectStreamPlayer::onPositionChanged(qint64 position)
{
    Q_UNUSED(position)
    // 位置情報は無限ストリームでは意味がないため特に処理しない
}

void DirectStreamPlayer::onBufferProgressChanged(float progress)
{
    int progressPercent = static_cast<int>(progress * 100);
    emit bufferStatusChanged(m_streamDevice->bufferSize(), progressPercent);
}

void DirectStreamPlayer::onStreamDeviceDataAvailable()
{
    // データが利用可能になった時の処理
    // Qt Multimediaが自動的にreadData()を呼び出すため、特別な処理は不要
}

void DirectStreamPlayer::onStreamDeviceBufferSizeChanged(qint64 size)
{
    m_lastBufferSize = size;
    int bufferStatus = calculateBufferStatus();
    emit bufferStatusChanged(size, bufferStatus);
}

void DirectStreamPlayer::checkPlaybackProgress()
{
    if (!m_streamingActive) return;

    // 再生状態ログ出力
    LOG_DEBUG(
        QString("Playback check - State: %1, Buffer: %2 bytes, Position: %3ms")
        .arg(static_cast<int>(m_mediaPlayer->playbackState()))
        .arg(m_streamDevice->bufferSize())
        .arg(m_mediaPlayer->position()));
}

void DirectStreamPlayer::autoStartPlayback()
{
    if (!m_streamingActive || m_hasStartedPlayback) {
        m_autoStartTimer->stop();
        return;
    }

    m_autoStartAttempts++;
    qint64 currentBufferSize = m_streamDevice->bufferSize();

    LOG_DEBUG(
        QString("Auto start attempt %1, buffer size: %2 bytes")
        .arg(m_autoStartAttempts).arg(currentBufferSize));

    // 十分なバッファが蓄積されたら再生開始
    if (currentBufferSize >= BUFFER_THRESHOLD) {
        if (m_mediaPlayer->playbackState() == QMediaPlayer::StoppedState) {
            m_mediaPlayer->play();
            LOG_INFO( 
                "Auto-started playback with buffer size: " + QString::number(currentBufferSize));
        }
    }

    // 最大試行回数に達したら諦める
    if (m_autoStartAttempts >= MAX_AUTO_START_ATTEMPTS) {
        m_autoStartTimer->stop();
        LOG_WARNING( 
            "Auto-start attempts exhausted, stopping timer");
    }
}

void DirectStreamPlayer::updateMediaInfo()
{
    QString info = QString("Buffer: %1 bytes, State: %2")
        .arg(m_streamDevice->bufferSize())
        .arg(static_cast<int>(m_mediaPlayer->playbackState()));

    emit mediaInfoChanged(info);
}

int DirectStreamPlayer::calculateBufferStatus() const
{
    qint64 bufferSize = m_streamDevice->bufferSize();
    if (bufferSize >= BUFFER_THRESHOLD) {
        return 100; // 十分なバッファ
    }
    return static_cast<int>((bufferSize * 100) / BUFFER_THRESHOLD);
}

void DirectStreamPlayer::startContinuousFFplayPlayback()
{
#ifdef USE_FFMPEG_EXTERNAL
    LOG_INFO("🎬 継続的TSファイル更新: 毎秒ファイル更新でFFplay再生");
    
    // 継続的にTSファイルを更新するタイマーを開始
    QTimer *updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &DirectStreamPlayer::startExternalFFplayPlayback);
    updateTimer->start(2000); // 2秒ごとに更新
    
    // 初回実行
    startExternalFFplayPlayback();
#else
    LOG_WARNING("⚠️ USE_FFMPEG_EXTERNALが無効のため継続的FFplayを使用できません");
#endif
}

void DirectStreamPlayer::startExternalFFplayPlayback()
{
#ifdef USE_FFMPEG_EXTERNAL
    LOG_INFO("🎬 外部FFplay: TSファイル保存→詳細解析→再生開始");
    
    // 現在のTSバッファをファイルに保存
    QString tsFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/tvtest_stream.ts";
    // TSバッファから一時的にデータを読み込み
    m_streamDevice->open(QIODevice::ReadOnly);
    QByteArray currentTsData = m_streamDevice->readAll();
    
    if (!currentTsData.isEmpty()) {
        // 詳細TSデータ解析
        LOG_INFO(QString("📊 TSデータ詳細解析開始: 全体サイズ=%1 bytes").arg(currentTsData.size()));
        
        // TSパケット数計算
        int totalPackets = currentTsData.size() / 188;
        int remainder = currentTsData.size() % 188;
        LOG_INFO(QString("📦 TSパケット総数: %1個 (余り: %2 bytes)").arg(totalPackets).arg(remainder));
        
        // 最初の数パケットを詳細解析
        int checkPackets = qMin(5, totalPackets);
        for (int i = 0; i < checkPackets; i++) {
            int packetOffset = i * 188;
            QByteArray packet = currentTsData.mid(packetOffset, 188);
            
            if (packet.size() >= 4) {
                uint8_t sync = static_cast<uint8_t>(packet[0]);
                uint8_t tei_pusi_tp = static_cast<uint8_t>(packet[1]);
                uint16_t pid = ((static_cast<uint8_t>(packet[1]) & 0x1F) << 8) | static_cast<uint8_t>(packet[2]);
                uint8_t tsc_afc_cc = static_cast<uint8_t>(packet[3]);
                
                LOG_INFO(QString("📦 パケット%1: SYNC=0x%2 PID=0x%3 CC=%4")
                    .arg(i+1)
                    .arg(sync, 2, 16, QChar('0'))
                    .arg(pid, 3, 16, QChar('0'))
                    .arg(tsc_afc_cc & 0x0F));
                
                // 特殊PID検出
                if (pid == 0x0000) LOG_INFO("  → PAT (Program Association Table)");
                else if (pid == 0x0001) LOG_INFO("  → CAT (Conditional Access Table)");
                else if ((pid >= 0x0010) && (pid <= 0x001F)) LOG_INFO("  → PMT候補 (Program Map Table)");
                else if (pid == 0x1FFF) LOG_INFO("  → NULL (Null Packet)");
                else LOG_INFO(QString("  → データPID (0x%1)").arg(pid, 3, 16, QChar('0')));
            }
        }
        
        // TSデータの先頭バイトを検証
        QString headerHex;
        for (int i = 0; i < qMin(32, currentTsData.size()); i++) {
            headerHex += QString("%1 ").arg(static_cast<uint8_t>(currentTsData[i]), 2, 16, QChar('0'));
            if ((i + 1) % 16 == 0) headerHex += "\n";
        }
        LOG_INFO(QString("🔍 TSデータヘッダー32bytes:\n%1").arg(headerHex));
        
        // TSファイル保存を廃止 - 直接ストリーミング方式に変更
        LOG_INFO(QString("🎬 直接ストリーミング開始: %1 bytes").arg(currentTsData.size()));
            
            // FFplayプロセス起動（継続的ストリーミング）
            QString ffplayPath = "C:/ffmpeg/bin/ffplay.exe";
            QStringList arguments;
            arguments << "-f" << "mpegts"; // MPEG-TS形式指定
            arguments << "-i" << "-"; // 標準入力から読み取り
            arguments << "-loglevel" << "debug"; // デバッグレベルに変更
            arguments << "-probesize" << "1048576"; // プローブサイズ増加（1MB）
            arguments << "-analyzeduration" << "10000000"; // 解析時間増加（10秒）
            arguments << "-fflags" << "+genpts"; // PTS生成
            arguments << "-avoid_negative_ts" << "make_zero";
            arguments << "-sync" << "video"; // ビデオ同期
            arguments << "-framedrop"; // フレームドロップ許可
            arguments << "-infbuf"; // 無限バッファ（ライブストリーム用）
            
            QProcess *ffplayProcess = new QProcess(this);
            
            // FFplayの出力をキャプチャ
            connect(ffplayProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, ffplayProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                    QByteArray output = ffplayProcess->readAllStandardOutput();
                    QByteArray error = ffplayProcess->readAllStandardError();
                    
                    LOG_INFO(QString("🎬 FFplay終了: コード=%1, ステータス=%2")
                        .arg(exitCode).arg(exitStatus == QProcess::NormalExit ? "Normal" : "Crash"));
                    
                    if (!output.isEmpty()) {
                        LOG_INFO(QString("📺 FFplay出力:\n%1").arg(QString::fromUtf8(output)));
                    }
                    if (!error.isEmpty()) {
                        LOG_WARNING(QString("⚠️ FFplayエラー:\n%1").arg(QString::fromUtf8(error)));
                    }
                    
                    ffplayProcess->deleteLater();
                });
            
            ffplayProcess->start(ffplayPath, arguments);
            
            if (ffplayProcess->waitForStarted()) {
                LOG_INFO("✅ FFplay起動成功（直接ストリーミングモード）");
                
                // 初期TSストリームデータを送信
                LOG_INFO("📡 初期TSストリーム送信開始");
                QByteArray tsData = currentTsData;
                qint64 written = ffplayProcess->write(tsData);
                LOG_INFO(QString("📡 初期TSストリーム送信: %1 bytes → FFplay stdin").arg(written));
                
                // 継続的ストリーミング用のタイマー設定
                m_ffplayProcess = ffplayProcess; // プロセス参照保持
                
                // データストリーム継続送信タイマー
                QTimer *streamTimer = new QTimer(this);
                connect(streamTimer, &QTimer::timeout, [this]() {
                    if (m_ffplayProcess && m_ffplayProcess->state() == QProcess::Running && m_streamDevice) {
                        // 新しいTSデータを取得して送信
                        qint64 bufferSize = m_streamDevice->bufferSize();
                        if (bufferSize > 0) {
                            QByteArray newTsData(bufferSize, 0);
                            qint64 readBytes = m_streamDevice->read(newTsData.data(), bufferSize);
                            if (readBytes > 0) {
                                newTsData.resize(readBytes);
                                qint64 written = m_ffplayProcess->write(newTsData);
                                LOG_DEBUG(QString("📡 継続ストリーム送信: %1 bytes").arg(written));
                            }
                        }
                    }
                });
                streamTimer->start(100); // 100msごとに新データ送信
            } else {
                LOG_CRITICAL("❌ FFplay起動失敗");
                QByteArray startError = ffplayProcess->readAllStandardError();
                if (!startError.isEmpty()) {
                    LOG_CRITICAL(QString("FFplay起動エラー詳細: %1").arg(QString::fromUtf8(startError)));
                }
            }
    } else {
        LOG_WARNING("⚠️ TSストリームデータが空です");
    }
#else
    LOG_WARNING("⚠️ USE_FFMPEG_EXTERNALが無効のため外部FFplayを使用できません");
#endif
}

void DirectStreamPlayer::resetForChannelChange()
{
    LOG_INFO("🔄 チャンネル変更: 再生状態リセット開始");
    
    // 再生開始フラグをリセット
    m_hasStartedPlayback = false;
    
    // バッファをクリア
    m_streamDevice->clearBuffer();
    
    // 総受信データ量をリセット
    m_totalDataReceived = 0;
    
    // 自動開始試行回数をリセット
    m_autoStartAttempts = 0;
    
    LOG_INFO("✅ チャンネル変更: 再生状態リセット完了 - 新チャンネルでFFplay再実行可能");
}