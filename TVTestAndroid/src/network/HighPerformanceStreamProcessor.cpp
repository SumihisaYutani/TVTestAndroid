#include "HighPerformanceStreamProcessor.h"
#include "utils/Logger.h"
#include <QThread>
#include <QElapsedTimer>
#include <cstdlib>
#include <cstring>

// Ring Buffer実装
HighPerformanceStreamProcessor::RingBuffer::RingBuffer()
{
    // 【最適化1】16MB Ring Buffer事前確保（Cache Line最適化）
#ifdef _WIN32
    buffer = (uint8_t*)_aligned_malloc(BUFFER_SIZE, 64);
#else
    buffer = (uint8_t*)aligned_alloc(64, BUFFER_SIZE);
#endif
    
    if (!buffer) {
        LOG_CRITICAL("Ring Buffer allocation failed: 16MB");
        throw std::bad_alloc();
    }
    
    LOG_INFO("✅ Ring Buffer initialized: 16MB with 64-byte alignment");
}

HighPerformanceStreamProcessor::RingBuffer::~RingBuffer()
{
    if (buffer) {
#ifdef _WIN32
        _aligned_free(buffer);
#else
        free(buffer);
#endif
        buffer = nullptr;
    }
}

uint8_t* HighPerformanceStreamProcessor::RingBuffer::getWritePtr(size_t requestSize)
{
    size_t currentWrite = writePos.load(std::memory_order_acquire);
    size_t currentRead = readPos.load(std::memory_order_acquire);
    
    // 循環バッファでの利用可能スペース計算
    size_t availableSpace;
    if (currentWrite >= currentRead) {
        availableSpace = BUFFER_SIZE - currentWrite + currentRead;
    } else {
        availableSpace = currentRead - currentWrite;
    }
    
    // 安全マージンを考慮（1KB）
    if (availableSpace <= requestSize + 1024) {
        return nullptr; // バッファ満杯
    }
    
    return &buffer[currentWrite];
}

const uint8_t* HighPerformanceStreamProcessor::RingBuffer::getReadPtr(size_t requestSize)
{
    size_t currentRead = readPos.load(std::memory_order_acquire);
    size_t currentWrite = writePos.load(std::memory_order_acquire);
    
    // 利用可能データサイズ計算
    size_t availableData;
    if (currentWrite >= currentRead) {
        availableData = currentWrite - currentRead;
    } else {
        availableData = BUFFER_SIZE - currentRead + currentWrite;
    }
    
    if (availableData < requestSize) {
        return nullptr; // データ不足
    }
    
    return &buffer[currentRead];
}

size_t HighPerformanceStreamProcessor::RingBuffer::getAvailableDataSize() const
{
    size_t currentRead = readPos.load(std::memory_order_acquire);
    size_t currentWrite = writePos.load(std::memory_order_acquire);
    
    if (currentWrite >= currentRead) {
        return currentWrite - currentRead;
    } else {
        return BUFFER_SIZE - currentRead + currentWrite;
    }
}

uint32_t HighPerformanceStreamProcessor::RingBuffer::getUsagePercent() const
{
    size_t dataSize = getAvailableDataSize();
    return static_cast<uint32_t>((dataSize * 100) / BUFFER_SIZE);
}

void HighPerformanceStreamProcessor::RingBuffer::advanceWritePos(size_t bytes)
{
    size_t currentPos = writePos.load(std::memory_order_acquire);
    size_t newPos = (currentPos + bytes) % BUFFER_SIZE;
    writePos.store(newPos, std::memory_order_release);
}

void HighPerformanceStreamProcessor::RingBuffer::advanceReadPos(size_t bytes)
{
    size_t currentPos = readPos.load(std::memory_order_acquire);
    size_t newPos = (currentPos + bytes) % BUFFER_SIZE;
    readPos.store(newPos, std::memory_order_release);
}

// メインクラス実装
HighPerformanceStreamProcessor::HighPerformanceStreamProcessor(QObject *parent)
    : QObject(parent)
{
    LOG_INFO("HighPerformanceStreamProcessor初期化開始（簡略版）");
}

HighPerformanceStreamProcessor::~HighPerformanceStreamProcessor()
{
    shutdown();
}

bool HighPerformanceStreamProcessor::initialize()
{
    LOG_INFO("=== HighPerformanceStreamProcessor 初期化（簡略版）===");
    
    try {
        // Ring Bufferは既にコンストラクタで初期化済み
        LOG_INFO("Ring Buffer状態確認完了");
        
        // プロセッサ初期化（簡略版）
        if (!initializeProcessor()) {
            LOG_CRITICAL("❌ Processor initialization failed");
            return false;
        }
        
        LOG_INFO("✅ HighPerformanceStreamProcessor initialization completed（簡略版）");
        return true;
        
    } catch (const std::exception& e) {
        LOG_CRITICAL(QString("❌ Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void HighPerformanceStreamProcessor::shutdown()
{
    LOG_INFO("=== HighPerformanceStreamProcessor 終了処理 ===");
    
    stopStreaming();
    shutdownProcessor();
    
    m_socket = nullptr;
    
    LOG_INFO("✅ HighPerformanceStreamProcessor shutdown completed");
}

bool HighPerformanceStreamProcessor::initializeProcessor()
{
    LOG_INFO("プロセッサ初期化開始（簡略版）");
    LOG_INFO("✅ プロセッサ初期化完了（Ring Buffer準備完了）");
    return true;
}

void HighPerformanceStreamProcessor::shutdownProcessor()
{
    LOG_INFO("✅ プロセッサ終了完了");
}

void HighPerformanceStreamProcessor::setSocket(QTcpSocket* socket)
{
    if (m_socket) {
        disconnect(m_socket, &QTcpSocket::readyRead, this, &HighPerformanceStreamProcessor::onSocketDataReady);
    }
    
    m_socket = socket;
    
    if (m_socket) {
        connect(m_socket, &QTcpSocket::readyRead, this, &HighPerformanceStreamProcessor::onSocketDataReady);
        LOG_INFO("✅ Socket connected to HighPerformanceStreamProcessor");
    }
}

void HighPerformanceStreamProcessor::startStreaming()
{
    LOG_INFO("🚀 ゼロコピー・リアルタイムストリーミング開始");
    
    m_isStreaming.store(true, std::memory_order_release);
    
    // 統計リセット
    m_stats.totalBytesReceived.store(0);
    m_stats.totalPacketsProcessed.store(0);
    m_stats.currentBitrate.store(0);
    m_stats.isRealTimeStreaming.store(true);
    
    emit realTimeStreamingStarted();
    
    LOG_INFO("✅ リアルタイムストリーミング開始完了");
}

void HighPerformanceStreamProcessor::stopStreaming()
{
    LOG_INFO("⏹️ ストリーミング停止");
    
    m_isStreaming.store(false, std::memory_order_release);
    m_stats.isRealTimeStreaming.store(false);
    
    LOG_INFO("✅ ストリーミング停止完了");
}

void HighPerformanceStreamProcessor::onSocketDataReady()
{
    if (!m_socket || !m_isStreaming.load(std::memory_order_acquire)) {
        return;
    }
    
    qint64 availableBytes = m_socket->bytesAvailable();
    if (availableBytes <= 0) {
        return;
    }
    
    // 【ゼロコピー最適化1】Socket→Ring Buffer直接書き込み
    uint8_t* writePtr = m_ringBuffer.getWritePtr(availableBytes);
    
    if (!writePtr) {
        // バッファ満杯時は古いデータを破棄して空間確保
        size_t discardSize = availableBytes + 4096; // 少し多めに確保
        m_ringBuffer.advanceReadPos(discardSize);
        writePtr = m_ringBuffer.getWritePtr(availableBytes);
        
        if (!writePtr) {
            LOG_WARNING("⚠️ Ring Buffer space allocation failed");
            return;
        }
    }
    
    // Socket → Ring Buffer 直接読み込み（メモリコピー1回のみ）
    qint64 bytesRead = m_socket->read(reinterpret_cast<char*>(writePtr), availableBytes);
    
    if (bytesRead > 0) {
        // 書き込み位置更新
        m_ringBuffer.advanceWritePos(bytesRead);
        
        // 統計更新
        m_stats.totalBytesReceived.fetch_add(bytesRead, std::memory_order_relaxed);
        m_stats.bufferUsagePercent.store(m_ringBuffer.getUsagePercent(), std::memory_order_relaxed);
        
        // 蓄積データの処理
        processAccumulatedData();
        
        // 統計更新（パフォーマンス考慮で1000回に1回）
        static int updateCounter = 0;
        if (++updateCounter % 1000 == 0) {
            updateStats();
        }
    }
}

// データ処理（簡略版）

void HighPerformanceStreamProcessor::processAccumulatedData()
{
    // Ring Bufferに十分なデータが蓄積されているかチェック
    size_t availableData = m_ringBuffer.getAvailableDataSize();
    
    const size_t MIN_PROCESS_SIZE = 188 * 10; // 最低10TSパケット
    
    if (availableData < MIN_PROCESS_SIZE) {
        return; // データ不足
    }
    
    // 簡略版：TSパケット処理
    while (availableData >= 188) {
        const uint8_t* readPtr = m_ringBuffer.getReadPtr(188);
        if (!readPtr) break;
        
        // TSパケット(188バイト)を処理
        QByteArray tsPacket(reinterpret_cast<const char*>(readPtr), 188);
        m_ringBuffer.advanceReadPos(188);
        
        // 統計更新
        m_stats.totalPacketsProcessed.fetch_add(1, std::memory_order_relaxed);
        
        // TSパケットシグナル発行（将来のFFmpeg統合用）
        emit tsPacketReady(tsPacket);
        
        availableData -= 188;
    }
}

void HighPerformanceStreamProcessor::updateStats()
{
    // ビットレート計算（簡易版）
    static QElapsedTimer timer;
    static uint64_t lastBytes = 0;
    
    if (!timer.isValid()) {
        timer.start();
        lastBytes = m_stats.totalBytesReceived.load();
        return;
    }
    
    qint64 elapsed = timer.restart();
    uint64_t currentBytes = m_stats.totalBytesReceived.load();
    
    if (elapsed > 0) {
        uint64_t bytesDiff = currentBytes - lastBytes;
        uint32_t bitrate = static_cast<uint32_t>((bytesDiff * 8 * 1000) / elapsed); // bps
        m_stats.currentBitrate.store(bitrate, std::memory_order_relaxed);
    }
    
    lastBytes = currentBytes;
    
    emit statsUpdated(m_stats);
}

