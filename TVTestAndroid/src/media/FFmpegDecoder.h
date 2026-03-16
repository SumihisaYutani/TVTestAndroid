#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <QObject>
#include <QByteArray>
#include <QImage>
#include <QMutex>
#include <QThread>

// FFmpeg C API headers
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}

/**
 * @brief FFmpegベースのTSストリームデコーダー
 * 
 * MPEG-2 Transport Streamをリアルタイムでデコードし、
 * QImageフレームとして出力するクラス
 */
class FFmpegDecoder : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief コンストラクタ
     */
    explicit FFmpegDecoder(QObject *parent = nullptr);
    
    /**
     * @brief デストラクタ
     */
    ~FFmpegDecoder();
    
    /**
     * @brief デコーダー初期化
     * @return 成功時true
     */
    bool initialize();
    
    /**
     * @brief TSデータの入力
     * @param tsData Transport Streamデータ
     */
    void inputTsData(const QByteArray &tsData);
    
    /**
     * @brief デコーダーのリセット
     */
    void reset();
    
    /**
     * @brief 現在のフレームサイズ取得
     */
    QSize getFrameSize() const { return QSize(m_frameWidth, m_frameHeight); }
    
    /**
     * @brief デコード統計情報
     */
    struct DecodeStats {
        int totalFrames;
        int droppedFrames;
        double frameRate;
        QString codecName;
    };
    
    DecodeStats getStats() const { return m_stats; }

signals:
    /**
     * @brief 新しいフレームが利用可能
     * @param frame デコードされたフレーム（RGB形式）
     */
    void frameReady(const QImage &frame);
    
    /**
     * @brief エラー発生
     * @param error エラーメッセージ
     */
    void errorOccurred(const QString &error);
    
    /**
     * @brief デコード統計更新
     */
    void statsUpdated(const DecodeStats &stats);

private:
    /**
     * @brief TSパケットの処理
     * @param packet 受信パケット
     */
    void processPacket(AVPacket *packet);
    
    /**
     * @brief フレームのデコード
     * @param frame デコードされたフレーム
     */
    void processFrame(AVFrame *frame);
    
    /**
     * @brief YUV→RGB変換
     * @param frame YUVフレーム
     * @return RGB QImage
     */
    QImage convertFrameToImage(AVFrame *frame);
    
    /**
     * @brief エラーハンドリング
     */
    void handleError(const QString &message, int errorCode = 0);

private:
    // FFmpeg コンテキスト
    AVFormatContext *m_formatContext;
    AVCodecContext *m_codecContext;
    AVCodec *m_codec;
    SwsContext *m_swsContext;
    
    // ストリーム情報
    int m_videoStreamIndex;
    int m_frameWidth;
    int m_frameHeight;
    AVPixelFormat m_pixelFormat;
    
    // バッファ管理
    QByteArray m_inputBuffer;
    AVIOContext *m_avioContext;
    uint8_t *m_ioBuffer;
    static const int IO_BUFFER_SIZE = 32768;
    
    // フレーム処理
    AVFrame *m_frame;
    AVFrame *m_rgbFrame;
    uint8_t *m_rgbBuffer;
    
    // 統計情報
    DecodeStats m_stats;
    
    // スレッドセーフティ
    QMutex m_mutex;
    
    // 初期化フラグ
    bool m_initialized;
    
    // カスタムIO読み取り関数
    static int readPacket(void *opaque, uint8_t *buf, int buf_size);
};

#endif // FFMPEGDECODER_H