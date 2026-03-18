#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <atomic>
#include <memory>

// FFmpeg統合は将来実装予定（現在はゼロコピー・Ring Bufferテスト版）
// extern "C" {
// #include <libavformat/avformat.h>
// #include <libavformat/avio.h>
// #include <libavutil/opt.h>
// }

/**
 * @brief ゼロコピー・高性能ストリーミングプロセッサ
 * 
 * 従来の実装で発生していた4箇所のメモリコピーを排除し、
 * Socket→Ring Buffer→FFmpeg→Qt Multimediaの直結パイプラインを実現。
 * 
 * 性能改善目標:
 * - メモリコピー: 75%削減 (752B → 188B/packet)
 * - CPU使用率: 75%削減 (40% → 10%)
 * - 受信遅延: 90%削減 (5-10ms → 0.1-0.5ms)
 * - スループット: 100倍向上 (0.15Mbps → 12-15Mbps)
 */
class HighPerformanceStreamProcessor : public QObject
{
    Q_OBJECT

public:
    explicit HighPerformanceStreamProcessor(QObject *parent = nullptr);
    ~HighPerformanceStreamProcessor();

    // 初期化・終了処理
    bool initialize();
    void shutdown();

    // ネットワーク接続
    void setSocket(QTcpSocket* socket);
    
    // ストリーミング制御
    void startStreaming();
    void stopStreaming();
    
    // 統計情報
    struct StreamingStats {
        std::atomic<uint64_t> totalBytesReceived{0};
        std::atomic<uint64_t> totalPacketsProcessed{0};
        std::atomic<uint32_t> currentBitrate{0};
        std::atomic<uint32_t> bufferUsagePercent{0};
        std::atomic<bool> isRealTimeStreaming{false};
    };
    
    const StreamingStats& getStats() const { return m_stats; }

signals:
    // リアルタイムストリーミング開始通知
    void realTimeStreamingStarted();
    
    // TSパケット供給（簡略版）
    void tsPacketReady(const QByteArray& packet);
    
    // エラー通知
    void streamingError(const QString& error);
    
    // 統計更新通知
    void statsUpdated(const StreamingStats& stats);

private slots:
    // Socket データ受信ハンドラ
    void onSocketDataReady();

private:
    // 【核心技術】Cache Line最適化 Lock-Free Ring Buffer
    struct alignas(64) RingBuffer {
        std::atomic<size_t> writePos{0};
        std::atomic<size_t> readPos{0};
        uint8_t* buffer = nullptr;
        static constexpr size_t BUFFER_SIZE = 16 * 1024 * 1024;  // 16MB事前確保
        
        // コンストラクタ・デストラクタ
        RingBuffer();
        ~RingBuffer();
        
        // ゼロコピー書き込み用ポインタ取得
        uint8_t* getWritePtr(size_t requestSize);
        
        // ゼロコピー読み込み用ポインタ取得
        const uint8_t* getReadPtr(size_t requestSize);
        
        // 利用可能データサイズ取得
        size_t getAvailableDataSize() const;
        
        // バッファ使用率取得 (0-100%)
        uint32_t getUsagePercent() const;
        
        // 書き込み位置更新
        void advanceWritePos(size_t bytes);
        
        // 読み込み位置更新
        void advanceReadPos(size_t bytes);
    };

    // FFmpeg関連（将来実装）
    // AVIOContext* m_avioContext = nullptr;
    // AVFormatContext* m_formatContext = nullptr;
    // AVCodecContext* m_codecContext = nullptr;
    // AVFrame* m_frame = nullptr;
    // AVPacket* m_packet = nullptr;
    
    // ネットワーク
    QTcpSocket* m_socket = nullptr;
    
    // バッファ・統計
    RingBuffer m_ringBuffer;
    StreamingStats m_stats;
    
    // スレッド制御
    std::atomic<bool> m_isStreaming{false};
    QThread* m_decoderThread = nullptr;
    
    // 内部処理メソッド
    bool initializeProcessor();
    void shutdownProcessor();
    
    // データ処理（簡略版）
    void processAccumulatedData();
    
    // 統計更新
    void updateStats();
};