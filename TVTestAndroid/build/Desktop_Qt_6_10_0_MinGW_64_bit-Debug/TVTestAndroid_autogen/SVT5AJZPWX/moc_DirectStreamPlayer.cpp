/****************************************************************************
** Meta object code from reading C++ file 'DirectStreamPlayer.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/media/DirectStreamPlayer.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DirectStreamPlayer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN18DirectStreamPlayerE_t {};
} // unnamed namespace

template <> constexpr inline auto DirectStreamPlayer::qt_create_metaobjectdata<qt_meta_tag_ZN18DirectStreamPlayerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DirectStreamPlayer",
        "playbackStateChanged",
        "",
        "QMediaPlayer::PlaybackState",
        "state",
        "mediaInfoChanged",
        "info",
        "errorOccurred",
        "error",
        "bufferStatusChanged",
        "bufferSize",
        "bufferStatus",
        "onPlaybackStateChanged",
        "onMediaStatusChanged",
        "QMediaPlayer::MediaStatus",
        "status",
        "onErrorOccurred",
        "QMediaPlayer::Error",
        "errorString",
        "onDurationChanged",
        "duration",
        "onPositionChanged",
        "position",
        "onBufferProgressChanged",
        "progress",
        "onStreamDeviceDataAvailable",
        "onStreamDeviceBufferSizeChanged",
        "size",
        "checkPlaybackProgress",
        "autoStartPlayback"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'playbackStateChanged'
        QtMocHelpers::SignalData<void(QMediaPlayer::PlaybackState)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'mediaInfoChanged'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'bufferStatusChanged'
        QtMocHelpers::SignalData<void(qint64, int)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::LongLong, 10 }, { QMetaType::Int, 11 },
        }}),
        // Slot 'onPlaybackStateChanged'
        QtMocHelpers::SlotData<void(QMediaPlayer::PlaybackState)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onMediaStatusChanged'
        QtMocHelpers::SlotData<void(QMediaPlayer::MediaStatus)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'onErrorOccurred'
        QtMocHelpers::SlotData<void(QMediaPlayer::Error, const QString &)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 17, 8 }, { QMetaType::QString, 18 },
        }}),
        // Slot 'onDurationChanged'
        QtMocHelpers::SlotData<void(qint64)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 20 },
        }}),
        // Slot 'onPositionChanged'
        QtMocHelpers::SlotData<void(qint64)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 22 },
        }}),
        // Slot 'onBufferProgressChanged'
        QtMocHelpers::SlotData<void(float)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Float, 24 },
        }}),
        // Slot 'onStreamDeviceDataAvailable'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStreamDeviceBufferSizeChanged'
        QtMocHelpers::SlotData<void(qint64)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 27 },
        }}),
        // Slot 'checkPlaybackProgress'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'autoStartPlayback'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DirectStreamPlayer, qt_meta_tag_ZN18DirectStreamPlayerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DirectStreamPlayer::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18DirectStreamPlayerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18DirectStreamPlayerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18DirectStreamPlayerE_t>.metaTypes,
    nullptr
} };

void DirectStreamPlayer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DirectStreamPlayer *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->playbackStateChanged((*reinterpret_cast<std::add_pointer_t<QMediaPlayer::PlaybackState>>(_a[1]))); break;
        case 1: _t->mediaInfoChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->bufferStatusChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 4: _t->onPlaybackStateChanged((*reinterpret_cast<std::add_pointer_t<QMediaPlayer::PlaybackState>>(_a[1]))); break;
        case 5: _t->onMediaStatusChanged((*reinterpret_cast<std::add_pointer_t<QMediaPlayer::MediaStatus>>(_a[1]))); break;
        case 6: _t->onErrorOccurred((*reinterpret_cast<std::add_pointer_t<QMediaPlayer::Error>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->onDurationChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 8: _t->onPositionChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 9: _t->onBufferProgressChanged((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 10: _t->onStreamDeviceDataAvailable(); break;
        case 11: _t->onStreamDeviceBufferSizeChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 12: _t->checkPlaybackProgress(); break;
        case 13: _t->autoStartPlayback(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DirectStreamPlayer::*)(QMediaPlayer::PlaybackState )>(_a, &DirectStreamPlayer::playbackStateChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DirectStreamPlayer::*)(const QString & )>(_a, &DirectStreamPlayer::mediaInfoChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DirectStreamPlayer::*)(const QString & )>(_a, &DirectStreamPlayer::errorOccurred, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DirectStreamPlayer::*)(qint64 , int )>(_a, &DirectStreamPlayer::bufferStatusChanged, 3))
            return;
    }
}

const QMetaObject *DirectStreamPlayer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DirectStreamPlayer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18DirectStreamPlayerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DirectStreamPlayer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void DirectStreamPlayer::playbackStateChanged(QMediaPlayer::PlaybackState _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void DirectStreamPlayer::mediaInfoChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void DirectStreamPlayer::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void DirectStreamPlayer::bufferStatusChanged(qint64 _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}
QT_WARNING_POP
