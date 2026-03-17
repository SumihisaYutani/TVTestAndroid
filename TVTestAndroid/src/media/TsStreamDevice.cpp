#include "TsStreamDevice.h"
#include "../utils/Logger.h"
#include <QMutexLocker>

TsStreamDevice::TsStreamDevice(QObject *parent)
    : QIODevice(parent)
    , m_readPosition(0)
    , m_streaming(false)
    , m_endOfStream(false)
{
    LOG_INFO("TsStreamDevice: Initialized memory-based TS stream device");
}

TsStreamDevice::~TsStreamDevice()
{
    close();
}

bool TsStreamDevice::isSequential() const
{
    // ストリーミングデバイスとして動作（シークできない）
    return true;
}

qint64 TsStreamDevice::readData(char *data, qint64 maxlen)
{
    QMutexLocker locker(&m_mutex);
    
    // 利用可能データがない場合
    if (m_readPosition >= m_buffer.size()) {
        if (!m_streaming || m_endOfStream) {
            // ストリーミング終了時
            return 0; // EOF
        }
        
        // データ到着を待機（最大100ms）
        m_waitCondition.wait(&m_mutex, 100);
        
        // 再チェック
        if (m_readPosition >= m_buffer.size()) {
            return 0; // タイムアウト
        }
    }
    
    // 読み取り可能サイズ計算
    qint64 availableData = m_buffer.size() - m_readPosition;
    qint64 readSize = qMin(maxlen, availableData);
    
    if (readSize > 0) {
        // データをコピー
        memcpy(data, m_buffer.constData() + m_readPosition, readSize);
        m_readPosition += readSize;
        
        LOG_DEBUG(QString("TsStreamDevice: Read %1 bytes from TS stream, position: %2/%3")
            .arg(readSize).arg(m_readPosition).arg(m_buffer.size()));
    }
    
    return readSize;
}

qint64 TsStreamDevice::writeData(const char *data, qint64 len)
{
    // このデバイスは読み取り専用
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
}

bool TsStreamDevice::canReadLine() const
{
    // TSストリームはライン単位ではない
    return false;
}

void TsStreamDevice::addTsData(const QByteArray &data)
{
    if (data.isEmpty()) return;
    
    QMutexLocker locker(&m_mutex);
    
    // 🔧 TSパケット同期検証: 188バイト単位で0x47チェック
    if (data.size() == 188) {
        // 単一TSパケットの場合
        if (static_cast<uint8_t>(data[0]) != 0x47) {
            LOG_WARNING(QString("⚠️ 不正TSパケット破棄: 先頭バイト=0x%1 (期待値: 0x47)")
                       .arg(static_cast<uint8_t>(data[0]), 2, 16, QChar('0')));
            return; // 破棄
        }
        m_buffer.append(data);
    } else {
        // 複数TSパケットまたは不完全データの場合
        int validBytes = 0;
        for (int i = 0; i < data.size() - 187; i += 188) {
            QByteArray packet = data.mid(i, 188);
            if (packet.size() == 188 && static_cast<uint8_t>(packet[0]) == 0x47) {
                m_buffer.append(packet);
                validBytes += 188;
            } else {
                LOG_WARNING(QString("⚠️ 不完全/不正TSパケット破棄: offset=%1, size=%2, 先頭=0x%3")
                           .arg(i).arg(packet.size())
                           .arg(packet.size() > 0 ? static_cast<uint8_t>(packet[0]) : 0, 2, 16, QChar('0')));
            }
        }
        
        if (validBytes > 0) {
            LOG_DEBUG(QString("TsStreamDevice: 検証済みTSデータ追加 %1/%2 bytes (有効/全体), バッファ合計: %3 bytes")
                     .arg(validBytes).arg(data.size()).arg(m_buffer.size()));
        }
    }
    
    // バッファサイズ制限
    limitBufferSize();
    
    // 待機中のreadData()に通知
    m_waitCondition.wakeAll();
    
    // シグナル発行 - readyReadのみ無効化（デッドロック回避）
    // emit readyRead(); // Qt Multimediaへの通知（デッドロック回避のため無効化）
    
    // UI更新用シグナルは非同期発行（デッドロック回避）
    QMetaObject::invokeMethod(this, [this, bufferSize = m_buffer.size()]() {
        emit dataAvailable();
        emit bufferSizeChanged(bufferSize);
    }, Qt::QueuedConnection);
}

void TsStreamDevice::clearBuffer()
{
    LOG_DEBUG("=== TsStreamDevice::clearBuffer() START ===");
    
    LOG_DEBUG("Acquiring mutex lock...");
    QMutexLocker locker(&m_mutex);
    LOG_DEBUG("Mutex lock acquired");
    
    LOG_DEBUG(QString("Buffer size before clear: %1 bytes").arg(m_buffer.size()));
    LOG_DEBUG(QString("Read position before clear: %1").arg(m_readPosition));
    
    m_buffer.clear();
    LOG_DEBUG("Buffer cleared");
    
    m_readPosition = 0;
    LOG_DEBUG("Read position reset to 0");
    
    LOG_DEBUG("Emitting bufferSizeChanged(0) signal - async");
    QMetaObject::invokeMethod(this, [this]() {
        emit bufferSizeChanged(0);
    }, Qt::QueuedConnection); // 非同期発行でデッドロック回避
    
    LOG_INFO("TsStreamDevice: Buffer cleared");
    LOG_DEBUG("=== TsStreamDevice::clearBuffer() END ===");
}

qint64 TsStreamDevice::bufferSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_buffer.size();
}

void TsStreamDevice::setStreaming(bool streaming)
{
    LOG_DEBUG(QString("=== TsStreamDevice::setStreaming(%1) START ===").arg(streaming ? "true" : "false"));
    
    LOG_DEBUG("Acquiring mutex lock...");
    QMutexLocker locker(&m_mutex);
    LOG_DEBUG("Mutex lock acquired");
    
    LOG_DEBUG(QString("Current streaming state: %1").arg(m_streaming ? "true" : "false"));
    m_streaming = streaming;
    LOG_DEBUG(QString("New streaming state: %1").arg(m_streaming ? "true" : "false"));
    
    if (!streaming) {
        LOG_DEBUG("Setting end of stream and waking waiting threads");
        m_endOfStream = true;
        m_waitCondition.wakeAll();
        LOG_DEBUG("End of stream set and threads woken");
    } else {
        LOG_DEBUG("Clearing end of stream flag");
        m_endOfStream = false;
        LOG_DEBUG("End of stream flag cleared");
    }
    
    LOG_INFO(QString("Streaming state changed: %1").arg(streaming ? "ON" : "OFF"));
    LOG_DEBUG("=== TsStreamDevice::setStreaming() END ===");
}

void TsStreamDevice::limitBufferSize()
{
    if (m_buffer.size() > MAX_BUFFER_SIZE) {
        // 古いデータを削除（前半を削除）
        qint64 removeSize = m_buffer.size() - (MAX_BUFFER_SIZE * 0.8); // 80%に縮小
        
        m_buffer.remove(0, removeSize);
        
        // 読み取り位置調整
        if (m_readPosition >= removeSize) {
            m_readPosition -= removeSize;
        } else {
            m_readPosition = 0;
        }
        
        LOG_WARNING(
            QString("Buffer size limited: removed %1 bytes, new size: %2")
            .arg(removeSize).arg(m_buffer.size()));
    }
}