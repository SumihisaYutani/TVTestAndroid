#include "FFmpegDecoder.h"
#include "../utils/Logger.h"
#include <QDebug>

FFmpegDecoder::FFmpegDecoder(QObject *parent)
    : QObject(parent)
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_codec(nullptr)
    , m_swsContext(nullptr)
    , m_videoStreamIndex(-1)
    , m_frameWidth(0)
    , m_frameHeight(0)
    , m_pixelFormat(AV_PIX_FMT_NONE)
    , m_avioContext(nullptr)
    , m_ioBuffer(nullptr)
    , m_frame(nullptr)
    , m_rgbFrame(nullptr)
    , m_rgbBuffer(nullptr)
    , m_initialized(false)
{
    // FFmpeg初期化（一度だけ実行）
    static bool ffmpegInitialized = false;
    if (!ffmpegInitialized) {
        av_register_all();
        avformat_network_init();
        av_log_set_level(AV_LOG_WARNING); // ログレベル設定
        ffmpegInitialized = true;
        LOG_INFO("FFmpeg初期化完了");
    }
    
    // 統計情報初期化
    m_stats = {0, 0, 0.0, ""};
}

FFmpegDecoder::~FFmpegDecoder()
{
    reset();
}

bool FFmpegDecoder::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        LOG_WARNING("FFmpegDecoder既に初期化済み");
        return true;
    }
    
    // IOバッファ確保
    m_ioBuffer = static_cast<uint8_t*>(av_malloc(IO_BUFFER_SIZE));
    if (!m_ioBuffer) {
        handleError("IOバッファ確保失敗");
        return false;
    }
    
    // カスタムIO Context作成
    m_avioContext = avio_alloc_context(
        m_ioBuffer, IO_BUFFER_SIZE, 0, this, readPacket, nullptr, nullptr);
    
    if (!m_avioContext) {
        handleError("AVIO Context作成失敗");
        return false;
    }
    
    // Format Context作成
    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) {
        handleError("Format Context作成失敗");
        return false;
    }
    
    m_formatContext->pb = m_avioContext;
    
    // Transport Stream形式を明示的に設定
    AVInputFormat *inputFormat = av_find_input_format("mpegts");
    if (!inputFormat) {
        handleError("MPEG-TS入力フォーマットが見つからない");
        return false;
    }
    
    // ストリーム情報の解析
    int ret = avformat_open_input(&m_formatContext, nullptr, inputFormat, nullptr);
    if (ret < 0) {
        handleError("入力ストリーム解析失敗", ret);
        return false;
    }
    
    ret = avformat_find_stream_info(m_formatContext, nullptr);
    if (ret < 0) {
        handleError("ストリーム情報取得失敗", ret);
        return false;
    }
    
    // ビデオストリーム検索
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            break;
        }
    }
    
    if (m_videoStreamIndex == -1) {
        handleError("ビデオストリームが見つからない");
        return false;
    }
    
    // コーデック情報取得
    AVCodecParameters *codecpar = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    m_codec = avcodec_find_decoder(codecpar->codec_id);
    if (!m_codec) {
        handleError(QString("コーデック未対応: ID=%1").arg(codecpar->codec_id));
        return false;
    }
    
    // コーデックコンテキスト作成
    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext) {
        handleError("Codec Context作成失敗");
        return false;
    }
    
    ret = avcodec_parameters_to_context(m_codecContext, codecpar);
    if (ret < 0) {
        handleError("Codecパラメータ設定失敗", ret);
        return false;
    }
    
    // コーデック初期化
    ret = avcodec_open2(m_codecContext, m_codec, nullptr);
    if (ret < 0) {
        handleError("Codec初期化失敗", ret);
        return false;
    }
    
    // フレーム情報設定
    m_frameWidth = m_codecContext->width;
    m_frameHeight = m_codecContext->height;
    m_pixelFormat = m_codecContext->pix_fmt;
    
    // フレームバッファ確保
    m_frame = av_frame_alloc();
    m_rgbFrame = av_frame_alloc();
    
    if (!m_frame || !m_rgbFrame) {
        handleError("フレームバッファ確保失敗");
        return false;
    }
    
    // RGB変換バッファ確保
    int rgbBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_frameWidth, m_frameHeight, 1);
    m_rgbBuffer = static_cast<uint8_t*>(av_malloc(rgbBufferSize));
    
    av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, 
                        m_rgbBuffer, AV_PIX_FMT_RGB24, m_frameWidth, m_frameHeight, 1);
    
    // スケーリングコンテキスト初期化
    m_swsContext = sws_getContext(
        m_frameWidth, m_frameHeight, m_pixelFormat,
        m_frameWidth, m_frameHeight, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    if (!m_swsContext) {
        handleError("スケーリングコンテキスト作成失敗");
        return false;
    }
    
    // 統計情報更新
    m_stats.codecName = QString::fromUtf8(m_codec->name);
    
    m_initialized = true;
    LOG_INFO(QString("FFmpegDecoder初期化完了: %1x%2, Codec=%3")
             .arg(m_frameWidth).arg(m_frameHeight).arg(m_stats.codecName));
    
    return true;
}

void FFmpegDecoder::inputTsData(const QByteArray &tsData)
{
    if (!m_initialized) {
        if (!initialize()) {
            return; // 初期化失敗
        }
    }
    
    QMutexLocker locker(&m_mutex);
    
    // 入力バッファにデータ追加
    m_inputBuffer.append(tsData);
    
    // パケット読み取りとデコード
    AVPacket packet;
    av_init_packet(&packet);
    
    int ret = av_read_frame(m_formatContext, &packet);
    while (ret >= 0) {
        if (packet.stream_index == m_videoStreamIndex) {
            processPacket(&packet);
        }
        av_packet_unref(&packet);
        ret = av_read_frame(m_formatContext, &packet);
    }
}

void FFmpegDecoder::processPacket(AVPacket *packet)
{
    int ret = avcodec_send_packet(m_codecContext, packet);
    if (ret < 0) {
        LOG_WARNING(QString("パケット送信失敗: %1").arg(ret));
        return;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecContext, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break; // もっとデータが必要、またはストリーム終了
        } else if (ret < 0) {
            LOG_WARNING(QString("フレーム受信失敗: %1").arg(ret));
            break;
        }
        
        processFrame(m_frame);
        m_stats.totalFrames++;
    }
}

void FFmpegDecoder::processFrame(AVFrame *frame)
{
    // YUV → RGB変換
    sws_scale(m_swsContext, frame->data, frame->linesize, 0, m_frameHeight,
              m_rgbFrame->data, m_rgbFrame->linesize);
    
    // QImageに変換
    QImage image = convertFrameToImage(m_rgbFrame);
    if (!image.isNull()) {
        emit frameReady(image);
    }
    
    // 統計更新（毎100フレーム）
    if (m_stats.totalFrames % 100 == 0) {
        emit statsUpdated(m_stats);
    }
}

QImage FFmpegDecoder::convertFrameToImage(AVFrame *frame)
{
    QImage image(frame->data[0], m_frameWidth, m_frameHeight, 
                 frame->linesize[0], QImage::Format_RGB888);
    
    return image.copy(); // ディープコピーで安全性確保
}

void FFmpegDecoder::reset()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_rgbBuffer) {
        av_free(m_rgbBuffer);
        m_rgbBuffer = nullptr;
    }
    
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    
    if (m_rgbFrame) {
        av_frame_free(&m_rgbFrame);
    }
    
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
    }
    
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
    }
    
    if (m_avioContext) {
        av_freep(&m_avioContext->buffer);
        av_freep(&m_avioContext);
    }
    
    m_inputBuffer.clear();
    m_initialized = false;
    
    LOG_INFO("FFmpegDecoder リセット完了");
}

void FFmpegDecoder::handleError(const QString &message, int errorCode)
{
    QString fullMessage = message;
    if (errorCode != 0) {
        char errorStr[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(errorCode, errorStr, sizeof(errorStr));
        fullMessage += QString(" (FFmpeg: %1)").arg(errorStr);
    }
    
    LOG_CRITICAL(QString("FFmpegDecoderエラー: %1").arg(fullMessage));
    emit errorOccurred(fullMessage);
}

// カスタムIO読み取り関数（staticメンバー）
int FFmpegDecoder::readPacket(void *opaque, uint8_t *buf, int buf_size)
{
    FFmpegDecoder *decoder = static_cast<FFmpegDecoder*>(opaque);
    
    QMutexLocker locker(&decoder->m_mutex);
    
    if (decoder->m_inputBuffer.isEmpty()) {
        return AVERROR(EAGAIN); // データ待ち
    }
    
    int readSize = qMin(buf_size, decoder->m_inputBuffer.size());
    memcpy(buf, decoder->m_inputBuffer.data(), readSize);
    decoder->m_inputBuffer.remove(0, readSize);
    
    return readSize;
}