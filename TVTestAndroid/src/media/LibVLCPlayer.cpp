#include "LibVLCPlayer.h"
#include "utils/Logger.h"

LibVLCPlayer::LibVLCPlayer(QObject *parent)
    : QObject(parent)
    , m_tsBuffer(new TsBuffer(this))
{}

LibVLCPlayer::~LibVLCPlayer()
{
    stop();
    if (m_player) libvlc_media_player_release(m_player);
    if (m_media)  libvlc_media_release(m_media);
    if (m_vlc)    libvlc_release(m_vlc);
}

bool LibVLCPlayer::init(WId windowId)
{
    const char *args[] = {
        "--no-video-title-show",
        "--network-caching=150",
        "--live-caching=150",
        "--clock-jitter=0",
    };
    m_vlc = libvlc_new(4, args);
    if (!m_vlc) {
        LOG_CRITICAL("libVLC初期化失敗");
        return false;
    }

    m_media = libvlc_media_new_callbacks(
        m_vlc,
        cbOpen, cbRead, cbSeek, cbClose,
        this
    );

    m_player = libvlc_media_player_new_from_media(m_media);

#if defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(m_player, (void*)windowId);
#elif defined(Q_OS_LINUX)
    libvlc_media_player_set_xwindow(m_player, (uint32_t)windowId);
#endif

    LOG_INFO("LibVLCPlayer初期化完了");
    return true;
}

void LibVLCPlayer::play()
{
    if (m_player) {
        libvlc_media_player_play(m_player);
        emit playing();
    }
}

void LibVLCPlayer::stop()
{
    if (m_player) {
        libvlc_media_player_stop(m_player);
        emit stopped();
    }
}

void LibVLCPlayer::clearBuffer()
{
    m_tsBuffer->clear();
}

int LibVLCPlayer::cbOpen(void *opaque, void **datap, uint64_t *sizep)
{
    *datap = opaque;
    *sizep = UINT64_MAX;
    return 0;
}

void LibVLCPlayer::cbClose(void *) {}

ssize_t LibVLCPlayer::cbRead(void *opaque, unsigned char *buf, size_t len)
{
    auto *self = static_cast<LibVLCPlayer*>(opaque);
    return self->m_tsBuffer->blockingRead(reinterpret_cast<char*>(buf), len);
}

int LibVLCPlayer::cbSeek(void *, uint64_t)
{
    return -1;
}
