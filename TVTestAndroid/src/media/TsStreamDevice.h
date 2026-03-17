#ifndef TSSTREAMDEVICE_H
#define TSSTREAMDEVICE_H

#include <QIODevice>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>

/**
 * @brief TSストリームデータをメモリ上で処理するQIODeviceサブクラス
 * 
 * ファイル保存を行わず、受信したTSストリームデータを直接Qt Multimediaに
 * 提供することで、元TVTest同様のシームレスな再生を実現する
 */
class TsStreamDevice : public QIODevice
{
    Q_OBJECT

public:
    explicit TsStreamDevice(QObject *parent = nullptr);
    ~TsStreamDevice();

    // QIODevice interface
    bool isSequential() const override;
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    bool canReadLine() const override;

    /**
     * @brief TSストリームデータを追加
     * @param data TSストリームデータ（188バイト単位のTSパケット推奨）
     */
    void addTsData(const QByteArray &data);

    /**
     * @brief バッファをクリア
     */
    void clearBuffer();

    /**
     * @brief 現在のバッファサイズを取得
     * @return バッファサイズ（バイト）
     */
    qint64 bufferSize() const;

    /**
     * @brief ストリーミング状態を設定
     * @param streaming ストリーミング中かどうか
     */
    void setStreaming(bool streaming);

signals:
    /**
     * @brief データが利用可能になった時に発行
     */
    void dataAvailable();

    /**
     * @brief バッファサイズが変更された時に発行
     * @param size 新しいバッファサイズ
     */
    void bufferSizeChanged(qint64 size);

private:
    QByteArray m_buffer;              // TSストリームバッファ
    qint64 m_readPosition;            // 現在の読み取り位置
    mutable QMutex m_mutex;           // スレッドセーフ用
    QWaitCondition m_waitCondition;   // データ待機用
    bool m_streaming;                 // ストリーミング状態
    bool m_endOfStream;               // ストリーム終了フラグ

    static const qint64 MAX_BUFFER_SIZE = 20 * 1024 * 1024; // 20MB制限
    static const qint64 MIN_READ_SIZE = 188 * 10;           // 最小読み取りサイズ

    /**
     * @brief バッファサイズを制限内に調整
     */
    void limitBufferSize();
};

#endif // TSSTREAMDEVICE_H