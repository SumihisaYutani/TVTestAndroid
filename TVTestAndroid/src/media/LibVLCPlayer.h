#pragma once
#include <QObject>
#include <vlc/vlc.h>
#include "TsBuffer.h"

class LibVLCPlayer : public QObject
{
    Q_OBJECT
public:
    explicit LibVLCPlayer(QObject *parent = nullptr);
    ~LibVLCPlayer();

    bool init(WId windowId);
    void play();
    void stop();
    void clearBuffer();

    TsBuffer *tsBuffer() const { return m_tsBuffer; }

signals:
    void playing();
    void stopped();

private:
    static int     cbOpen (void *opaque, void **datap, uint64_t *sizep);
    static void    cbClose(void *opaque);
    static ssize_t cbRead (void *opaque, unsigned char *buf, size_t len);
    static int     cbSeek (void *opaque, uint64_t offset);

    libvlc_instance_t     *m_vlc    = nullptr;
    libvlc_media_t        *m_media  = nullptr;
    libvlc_media_player_t *m_player = nullptr;
    TsBuffer              *m_tsBuffer;
};
