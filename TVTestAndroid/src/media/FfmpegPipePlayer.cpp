#include "FfmpegPipePlayer.h"
#include "utils/Logger.h"

FfmpegPipePlayer::FfmpegPipePlayer(QObject *parent)
    : QObject(parent)
    , m_tsBuffer(new TsBuffer(this))
    , m_process(new QProcess(this))
    , m_writerThread(new QThread(this))
    , m_worker(nullptr)
{
    connect(m_process, &QProcess::errorOccurred,
            this, &FfmpegPipePlayer::onProcessError);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FfmpegPipePlayer::onProcessFinished);
}

FfmpegPipePlayer::~FfmpegPipePlayer()
{
    stop();
}

bool FfmpegPipePlayer::init(QWidget *videoWidget)
{
    m_videoWidget = videoWidget;

    // WA_NativeWindow でwinIdを安定させる
    m_videoWidget->setAttribute(Qt::WA_NativeWindow);
    m_videoWidget->winId(); // 強制的にネイティブウィンドウ作成

    m_windowId = m_videoWidget->winId();

    LOG_INFO(QString("FfmpegPipePlayer初期化: winId=0x%1 size=%2x%3")
             .arg((qulonglong)m_windowId, 0, 16)
             .arg(videoWidget->width())
             .arg(videoWidget->height()));
    return true;
}

void FfmpegPipePlayer::play()
{
    if (m_process->state() == QProcess::Running) {
        LOG_INFO("既に再生中");
        return;
    }

    // winIdを最新値に更新（ウィンドウ表示後に変わる場合がある）
    if (m_videoWidget) {
        m_windowId = m_videoWidget->winId();
        LOG_INFO(QString("再生開始時winId=0x%1").arg((qulonglong)m_windowId, 0, 16));
    }

    int w = m_videoWidget ? m_videoWidget->width()  : 640;
    int h = m_videoWidget ? m_videoWidget->height() : 360;

    QStringList args;
    args << "-f"               << "mpegts"
         << "-probesize"       << "500000"
         << "-analyzeduration" << "500000"
         << "-fflags"          << "+genpts+discardcorrupt"
         << "-flags"           << "low_delay"
         << "-avioflags"       << "direct"
         << "-i"               << "pipe:0"
         << "-vf"              << "setpts=0"
         << "-sync"            << "ext"
         << "-noborder"
         << "-window_title"    << ""
         << "-x"               << QString::number(w)
         << "-y"               << QString::number(h);

#ifdef Q_OS_WIN
    args << "-wid" << QString("0x%1").arg((qulonglong)m_windowId, 0, 16);
#endif

    m_process->setProgram("C:/ffmpeg/bin/ffplay.exe");
    m_process->setArguments(args);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->start();

    if (!m_process->waitForStarted(3000)) {
        LOG_CRITICAL("ffplay起動失敗: " + m_process->errorString());
        emit errorOccurred("ffplay起動失敗: " + m_process->errorString());
        return;
    }

    LOG_INFO("✅ ffplay起動成功");

    // パイプ書き込みワーカーを別スレッドで起動
    m_worker = new PipeWriterWorker(m_tsBuffer, m_process);
    m_worker->moveToThread(m_writerThread);
    connect(m_writerThread, &QThread::started,
            m_worker, &PipeWriterWorker::run);
    m_writerThread->start();

    emit playing();
}

void FfmpegPipePlayer::stop()
{
    if (m_worker) {
        m_worker->stop();
    }
    if (m_writerThread->isRunning()) {
        m_writerThread->quit();
        m_writerThread->wait(2000);
    }
    if (m_worker) {
        delete m_worker;
        m_worker = nullptr;
    }

    if (m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000))
            m_process->kill();
    }

    m_tsBuffer->clear();
    emit stopped();
    LOG_INFO("✅ ffplay停止");
}

void FfmpegPipePlayer::clearBuffer()
{
    m_tsBuffer->clear();
}

void FfmpegPipePlayer::resizeVideo(int w, int h)
{
    // ffplayはリサイズコマンドを持たないので再起動で対応
    if (m_process->state() != QProcess::Running) return;

    LOG_INFO(QString("リサイズ: %1x%2 → ffplay再起動").arg(w).arg(h));

    // ワーカーは止めずにffplayだけ再起動
    m_process->terminate();
    m_process->waitForFinished(1000);

    // 新サイズで再起動
    int savedW = m_videoWidget ? m_videoWidget->width()  : w;
    int savedH = m_videoWidget ? m_videoWidget->height() : h;

    QStringList args;
    args << "-f"               << "mpegts"
         << "-probesize"       << "500000"
         << "-analyzeduration" << "500000"
         << "-fflags"          << "+genpts+discardcorrupt"
         << "-flags"           << "low_delay"
         << "-avioflags"       << "direct"
         << "-i"               << "pipe:0"
         << "-vf"              << "setpts=0"
         << "-sync"            << "ext"
         << "-noborder"
         << "-window_title"    << ""
         << "-x"               << QString::number(savedW)
         << "-y"               << QString::number(savedH);

#ifdef Q_OS_WIN
    args << "-wid" << QString("0x%1").arg((qulonglong)m_windowId, 0, 16);
#endif

    m_process->setArguments(args);
    m_process->start();

    if (!m_process->waitForStarted(3000)) {
        LOG_CRITICAL("ffplayリサイズ再起動失敗");
        return;
    }

    LOG_INFO("✅ ffplayリサイズ再起動成功");
}

void FfmpegPipePlayer::onProcessError(QProcess::ProcessError error)
{
    QString msg = QString("ffplayエラー: %1").arg(m_process->errorString());
    LOG_CRITICAL(msg);
    emit errorOccurred(msg);
}

void FfmpegPipePlayer::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    LOG_INFO(QString("ffplay終了: exitCode=%1").arg(exitCode));
    emit stopped();
}
