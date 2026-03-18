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

    void clear()
    {
        QMutexLocker lock(&m_mutex);
        m_buffer.clear();
    }

private:
    QByteArray     m_buffer;
    QMutex         m_mutex;
    QWaitCondition m_cond;
};
