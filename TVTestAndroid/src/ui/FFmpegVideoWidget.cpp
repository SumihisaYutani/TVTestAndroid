#include "FFmpegVideoWidget.h"
#include "../utils/Logger.h"
#include <QPaintEvent>
#include <QResizeEvent>

FFmpegVideoWidget::FFmpegVideoWidget(QWidget *parent)
    : QWidget(parent)
    , m_keepAspectRatio(true)
    , m_frameCount(0)
    , m_noSignalText("信号なし - チャンネル設定後にTSストリームを受信します")
{
    // 背景色設定
    setStyleSheet("background-color: black;");
    setAttribute(Qt::WA_OpaquePaintEvent);
    
    // 最小サイズ設定
    setMinimumSize(320, 240);
    
    // FPS計算タイマー
    m_fpsTimer = new QTimer(this);
    connect(m_fpsTimer, &QTimer::timeout, this, &FFmpegVideoWidget::updateFpsStats);
    m_fpsTimer->start(1000); // 1秒間隔
    
    // 統計初期化
    m_stats = {0, 0.0, QSize(0, 0), QSize(0, 0)};
    
    LOG_INFO("FFmpegVideoWidget初期化完了");
}

void FFmpegVideoWidget::displayFrame(const QImage &frame)
{
    QMutexLocker locker(&m_frameMutex);
    
    if (frame.isNull()) {
        LOG_WARNING("無効なフレームを受信");
        return;
    }
    
    m_currentFrame = frame;
    m_frameCount++;
    
    // 統計更新
    if (m_stats.frameSize != frame.size()) {
        m_stats.frameSize = frame.size();
        LOG_INFO(QString("フレームサイズ変更: %1x%2")
                 .arg(frame.width()).arg(frame.height()));
    }
    
    // 再描画
    update();
    
    emit frameDisplayed();
}

void FFmpegVideoWidget::clearDisplay()
{
    QMutexLocker locker(&m_frameMutex);
    
    m_currentFrame = QImage();
    m_scaledFrame = QImage();
    update();
    
    LOG_INFO("表示クリア完了");
}

void FFmpegVideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QMutexLocker locker(&m_frameMutex);
    
    if (m_currentFrame.isNull()) {
        // 信号なし表示
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14));
        painter.drawText(rect(), Qt::AlignCenter, m_noSignalText);
        return;
    }
    
    // フレームのスケーリング計算
    QRect targetRect = calculateScaledRect(m_currentFrame);
    
    // 背景を黒で塗りつぶし
    painter.fillRect(rect(), Qt::black);
    
    // フレーム描画
    painter.drawImage(targetRect, m_currentFrame);
    
    // デバッグ情報表示（オプション）
    if (m_stats.displayFps > 0) {
        painter.setPen(Qt::yellow);
        painter.setFont(QFont("Arial", 10));
        QString debugText = QString("FPS: %1 | Frame: %2x%3")
                           .arg(m_stats.displayFps, 0, 'f', 1)
                           .arg(m_currentFrame.width())
                           .arg(m_currentFrame.height());
        painter.drawText(10, 20, debugText);
    }
    
    // 統計更新
    m_stats.scaledSize = targetRect.size();
    m_stats.displayedFrames = m_frameCount;
}

void FFmpegVideoWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // リサイズ時に再描画
    update();
    
    LOG_DEBUG(QString("VideoWidget リサイズ: %1x%2")
              .arg(event->size().width()).arg(event->size().height()));
}

QRect FFmpegVideoWidget::calculateScaledRect(const QImage &image) const
{
    if (image.isNull()) {
        return QRect();
    }
    
    QSize widgetSize = size();
    QSize imageSize = image.size();
    
    if (!m_keepAspectRatio) {
        // アスペクト比無視でウィジェット全体に拡大
        return rect();
    }
    
    // アスペクト比を保持してスケーリング
    double widgetAspect = static_cast<double>(widgetSize.width()) / widgetSize.height();
    double imageAspect = static_cast<double>(imageSize.width()) / imageSize.height();
    
    QSize scaledSize;
    
    if (imageAspect > widgetAspect) {
        // 横長画像：幅をウィジェットに合わせる
        scaledSize.setWidth(widgetSize.width());
        scaledSize.setHeight(static_cast<int>(widgetSize.width() / imageAspect));
    } else {
        // 縦長画像：高さをウィジェットに合わせる
        scaledSize.setHeight(widgetSize.height());
        scaledSize.setWidth(static_cast<int>(widgetSize.height() * imageAspect));
    }
    
    // 中央配置
    QPoint topLeft(
        (widgetSize.width() - scaledSize.width()) / 2,
        (widgetSize.height() - scaledSize.height()) / 2
    );
    
    return QRect(topLeft, scaledSize);
}

void FFmpegVideoWidget::updateFpsStats()
{
    static int lastFrameCount = 0;
    int currentFrameCount = m_frameCount;
    
    m_stats.displayFps = currentFrameCount - lastFrameCount;
    lastFrameCount = currentFrameCount;
    
    emit displayStatsUpdated(m_stats);
    
    // FPS情報をログ出力（10秒間隔）
    static int logCounter = 0;
    if (++logCounter >= 10 && m_stats.displayFps > 0) {
        LOG_INFO(QString("表示統計: FPS=%1, 総フレーム=%2, サイズ=%3x%4")
                 .arg(m_stats.displayFps, 0, 'f', 1)
                 .arg(m_stats.displayedFrames)
                 .arg(m_stats.frameSize.width())
                 .arg(m_stats.frameSize.height()));
        logCounter = 0;
    }
}

void FFmpegVideoWidget::updateStats()
{
    // 内部統計更新（必要に応じて拡張）
}