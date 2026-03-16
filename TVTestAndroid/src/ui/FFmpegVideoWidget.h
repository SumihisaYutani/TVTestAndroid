#ifndef FFMPEGVIDEOWIDGET_H
#define FFMPEGVIDEOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QPainter>
#include <QMutex>
#include <QTimer>

/**
 * @brief FFmpegDecoder用の動画表示ウィジェット
 * 
 * FFmpegDecoderからのQImageフレームを受信して表示するウィジェット
 */
class FFmpegVideoWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief コンストラクタ
     */
    explicit FFmpegVideoWidget(QWidget *parent = nullptr);
    
    /**
     * @brief アスペクト比維持設定
     */
    void setKeepAspectRatio(bool keep) { m_keepAspectRatio = keep; update(); }
    
    /**
     * @brief 現在表示中のフレーム取得
     */
    QImage getCurrentFrame() const { return m_currentFrame; }
    
    /**
     * @brief 表示統計情報
     */
    struct DisplayStats {
        int displayedFrames;
        double displayFps;
        QSize frameSize;
        QSize scaledSize;
    };
    
    DisplayStats getDisplayStats() const { return m_stats; }

public slots:
    /**
     * @brief 新しいフレーム表示
     * @param frame 表示するフレーム
     */
    void displayFrame(const QImage &frame);
    
    /**
     * @brief 表示クリア
     */
    void clearDisplay();

signals:
    /**
     * @brief フレーム表示完了
     */
    void frameDisplayed();
    
    /**
     * @brief 表示統計更新
     */
    void displayStatsUpdated(const DisplayStats &stats);

protected:
    /**
     * @brief 描画イベント
     */
    void paintEvent(QPaintEvent *event) override;
    
    /**
     * @brief リサイズイベント
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /**
     * @brief FPS計算タイマー
     */
    void updateFpsStats();

private:
    /**
     * @brief スケーリング計算
     */
    QRect calculateScaledRect(const QImage &image) const;
    
    /**
     * @brief 統計情報更新
     */
    void updateStats();

private:
    // 表示フレーム
    QImage m_currentFrame;
    QImage m_scaledFrame;
    
    // レイアウト設定
    bool m_keepAspectRatio;
    
    // 統計情報
    DisplayStats m_stats;
    QTimer *m_fpsTimer;
    int m_frameCount;
    
    // スレッドセーフティ
    mutable QMutex m_frameMutex;
    
    // 背景表示
    QString m_noSignalText;
};

#endif // FFMPEGVIDEOWIDGET_H