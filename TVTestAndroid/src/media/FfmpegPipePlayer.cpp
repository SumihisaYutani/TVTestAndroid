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
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FfmpegPipePlayer::onProcessFinished);
}

FfmpegPipePlayer::~FfmpegPipePlayer()
{
    stop();
}

bool FfmpegPipePlayer::init(WId windowId)
{
    m_windowId = windowId;
    LOG_INFO(QString("FfmpegPipePlayer初期化: windowId=%1").arg((qulonglong)windowId));
    return true;
}

void FfmpegPipePlayer::play()
{
    if (m_process->state() == QProcess::Running) {
        LOG_INFO("既に再生中");
        return;
    }

    // ffplayへの引数設定
    // -i pipe:0 → 標準入力からTSを読む
    // -window_title "" → タイトルバーなし
    // -left / -top → ウィンドウ位置（winIdに合わせる）
    QStringList args;
    args << "-f"              << "mpegts"
         << "-probesize"     << "500000"
         << "-analyzeduration" << "500000"
         << "-fflags"        << "+genpts+discardcorrupt"
         << "-flags"         << "low_delay"
         << "-avioflags"     << "direct"
         << "-i"             << "pipe:0"
         << "-vf"            << "setpts=0"   // 低遅延
         << "-sync"          << "ext"
         << "-autoexit"
         << "-window_title"  << "TVTest"
         << "-noborder";

#ifdef Q_OS_WIN
    // ffplayをウィジェット内に埋め込む（Windows）
    // WIDを16進文字列で渡す
    args << "-wid" << QString("0x%1").arg((qulonglong)m_windowId, 0, 16);
#endif

    m_process->setProgram("C:/ffmpeg/bin/ffplay.exe");
    m_process->setArguments(args);

    // 標準入力をパイプモードに設定
    m_process->setStandardInputFile(QProcess::nullDevice()); // 一旦null
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
    // ワーカー停止
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

    // ffplay停止
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
