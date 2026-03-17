#ifndef DIRECTSTREAMPLAYER_H
#define DIRECTSTREAMPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QTimer>
#include <QProcess>
#include "TsStreamDevice.h"

#ifdef USE_FFMPEG
#include "FFmpegDecoder.h"
#include "ui/FFmpegVideoWidget.h"
#endif

/**
 * @brief 直接ストリーミング再生クラス
 * 
 * 元TVTestのようにファイルを介さず、メモリ上のTSストリームを
 * 直接Qt Multimediaで再生するクラス
 */
class DirectStreamPlayer : public QObject
{
    Q_OBJECT

public:
    explicit DirectStreamPlayer(QObject *parent = nullptr);
    ~DirectStreamPlayer();

    /**
     * @brief 動画表示ウィジェットを設定
     * @param videoWidget 表示先ウィジェット
     */
    void setVideoWidget(QVideoWidget *videoWidget);

    /**
     * @brief TSストリームデータを追加（元TVTestと同じインターフェース）
     * @param tsData TSストリームデータ
     */
    void addTsStream(const QByteArray &tsData);

    /**
     * @brief ストリーミング再生を開始
     */
    void startStreaming();

    /**
     * @brief ストリーミング再生を停止
     */
    void stopStreaming();

    /**
     * @brief 一時停止
     */
    void pauseStreaming();

    /**
     * @brief 再生再開
     */
    void resumeStreaming();

    /**
     * @brief 再生状態を取得
     * @return 現在の再生状態
     */
    QMediaPlayer::PlaybackState playbackState() const;

    /**
     * @brief 音量を設定
     * @param volume 音量（0.0-1.0）
     */
    void setVolume(float volume);

    /**
     * @brief ミュート設定
     * @param muted ミュート状態
     */
    void setMuted(bool muted);

    /**
     * @brief TSストリームバッファ情報を取得
     * @return バッファサイズ（バイト）
     */
    qint64 bufferSize() const;

    /**
     * @brief チャンネル変更時の再生状態リセット
     */
    void resetForChannelChange();

signals:
    /**
     * @brief 再生状態変更時に発行
     * @param state 新しい再生状態
     */
    void playbackStateChanged(QMediaPlayer::PlaybackState state);

    /**
     * @brief メディア情報変更時に発行（解像度、ビットレートなど）
     * @param info メディア情報文字列
     */
    void mediaInfoChanged(const QString &info);

    /**
     * @brief エラー発生時に発行
     * @param error エラー内容
     */
    void errorOccurred(const QString &error);

    /**
     * @brief バッファ状態変更時に発行
     * @param bufferSize 現在のバッファサイズ
     * @param bufferStatus バッファ状態（0-100%）
     */
    void bufferStatusChanged(qint64 bufferSize, int bufferStatus);

private slots:
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onErrorOccurred(QMediaPlayer::Error error, const QString &errorString);
    void onDurationChanged(qint64 duration);
    void onPositionChanged(qint64 position);
    void onBufferProgressChanged(float progress);
    void onStreamDeviceDataAvailable();
    void onStreamDeviceBufferSizeChanged(qint64 size);
    void checkPlaybackProgress();
    void autoStartPlayback();

private:
    QMediaPlayer *m_mediaPlayer;        // Qt Multimedia プレイヤー
    QAudioOutput *m_audioOutput;        // 音声出力
    TsStreamDevice *m_streamDevice;     // TSストリームデバイス
    QVideoWidget *m_videoWidget;        // 動画表示ウィジェット

#ifdef USE_FFMPEG
    FFmpegDecoder *m_ffmpegDecoder;     // FFmpegデコーダー
    FFmpegVideoWidget *m_ffmpegVideoWidget; // FFmpeg動画表示ウィジェット
#endif

    QTimer *m_progressTimer;            // 再生進行監視タイマー
    QTimer *m_autoStartTimer;           // 自動開始タイマー
    
    bool m_streamingActive;             // ストリーミング活動状態
    qint64 m_lastBufferSize;            // 前回のバッファサイズ
    qint64 m_totalDataReceived;         // 受信総データ量
    int m_autoStartAttempts;            // 自動開始試行回数
    bool m_hasStartedPlayback;          // 再生開始フラグ
    QProcess *m_ffplayProcess;          // FFplay外部プロセス参照

    static const int BUFFER_THRESHOLD = 188 * 2000; // 自動開始に必要なバッファサイズ（376KB に増加）
    static const int MAX_AUTO_START_ATTEMPTS = 10;  // 最大自動開始試行回数

    /**
     * @brief 継続的TSファイル更新とFFplay再生開始
     */
    void startContinuousFFplayPlayback();

    /**
     * @brief 外部FFplayプロセスによる再生開始
     */
    void startExternalFFplayPlayback();

    /**
     * @brief TSストリームをファイルに保存
     */
    void saveTsStreamToFile();

    /**
     * @brief メディア情報を更新
     */
    void updateMediaInfo();

    /**
     * @brief バッファ状態を計算
     * @return バッファ充填率（0-100）
     */
    int calculateBufferStatus() const;
};

#endif // DIRECTSTREAMPLAYER_H