#pragma once
#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>

class TsBuffer : public QObject
{
    Q_OBJECT
public:
    explicit TsBuffer(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void appendData(const QByteArray &data)
    {
        QMutexLocker lock(&m_mutex);
        m_buffer.append(data);
        m_cond.wakeAll();
    }

public:
    // libVLC向け（将来用・既存）
    ssize_t blockingRead(char *buf, size_t len)
    {
        QMutexLocker lock(&m_mutex);
        if (m_buffer.isEmpty())
            m_cond.wait(&m_mutex, 500);
        if (m_buffer.isEmpty())
            return 0;
        qint64 n = qMin((qint64)len, (qint64)m_buffer.size());
        memcpy(buf, m_buffer.constData(), n);
        m_buffer.remove(0, n);
        return n;
    }

    // FFmpegパイプ向け（追加）
    QByteArray take(int maxSize = 65536)
    {
        QMutexLocker lock(&m_mutex);
        if (m_buffer.isEmpty())
            m_cond.wait(&m_mutex, 200);
        if (m_buffer.isEmpty())
            return {};
        int n = qMin(maxSize, m_buffer.size());
        QByteArray result = m_buffer.left(n);
        m_buffer.remove(0, n);
        return result;
    }

    void clear()
    {
        QMutexLocker lock(&m_mutex);
        m_buffer.clear();
    }

    int size() const
    {
        QMutexLocker lock(&m_mutex);
        return m_buffer.size();
    }

private:
    QByteArray     m_buffer;
    mutable QMutex m_mutex;  // size()のconst対応でmutableに変更
    QWaitCondition m_cond;
};
