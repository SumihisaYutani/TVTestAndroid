#pragma once
#include <QObject>
#include <QProcess>
#include <QThread>
#include <QWidget>
#include "TsBuffer.h"

class PipeWriterWorker : public QObject
{
    Q_OBJECT
public:
    explicit PipeWriterWorker(TsBuffer *buffer, QProcess *process, QObject *parent = nullptr)
        : QObject(parent), m_buffer(buffer), m_process(process) {}

public slots:
    void run()
    {
        m_running = true;
        while (m_running) {
            if (m_process->state() != QProcess::Running) break;
            m_buffer->writeTo(m_process);
        }
    }
    void stop() { m_running = false; }

private:
    TsBuffer  *m_buffer;
    QProcess  *m_process;
    bool       m_running = false;
};

class FfmpegPipePlayer : public QObject
{
    Q_OBJECT
public:
    explicit FfmpegPipePlayer(QObject *parent = nullptr);
    ~FfmpegPipePlayer();

    bool init(QWidget *videoWidget);   // ウィジェットごと受け取る
    void play();
    void stop();
    void clearBuffer();
    void resizeVideo(int w, int h);    // リサイズ追従

    TsBuffer *tsBuffer() const { return m_tsBuffer; }

signals:
    void playing();
    void stopped();
    void errorOccurred(const QString &msg);

private slots:
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    TsBuffer         *m_tsBuffer;
    QProcess         *m_process;
    QThread          *m_writerThread;
    PipeWriterWorker *m_worker;
    QWidget          *m_videoWidget = nullptr;
    WId               m_windowId   = 0;
};
