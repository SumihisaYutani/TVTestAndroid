/****************************************************************************
** Meta object code from reading C++ file 'CoreEngine.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/core/CoreEngine.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CoreEngine.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10CoreEngineE_t {};
} // unnamed namespace

template <> constexpr inline auto CoreEngine::qt_create_metaobjectdata<qt_meta_tag_ZN10CoreEngineE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "CoreEngine",
        "stateChanged",
        "",
        "EngineState",
        "newState",
        "channelChanged",
        "ChannelInfo",
        "channel",
        "signalLevelChanged",
        "level",
        "errorOccurred",
        "error",
        "epgUpdated",
        "onNetworkStreamReady",
        "url",
        "onNetworkError",
        "updateStatus",
        "onMediaPlayerStateChanged",
        "QMediaPlayer::PlaybackState",
        "state",
        "onMediaPlayerError",
        "QMediaPlayer::Error"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'stateChanged'
        QtMocHelpers::SignalData<void(EngineState)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'channelChanged'
        QtMocHelpers::SignalData<void(const ChannelInfo &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Signal 'signalLevelChanged'
        QtMocHelpers::SignalData<void(int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Signal 'epgUpdated'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onNetworkStreamReady'
        QtMocHelpers::SlotData<void(const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 },
        }}),
        // Slot 'onNetworkError'
        QtMocHelpers::SlotData<void(const QString &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Slot 'updateStatus'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMediaPlayerStateChanged'
        QtMocHelpers::SlotData<void(QMediaPlayer::PlaybackState)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 18, 19 },
        }}),
        // Slot 'onMediaPlayerError'
        QtMocHelpers::SlotData<void(QMediaPlayer::Error)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 11 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CoreEngine, qt_meta_tag_ZN10CoreEngineE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject CoreEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10CoreEngineE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10CoreEngineE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10CoreEngineE_t>.metaTypes,
    nullptr
} };

void CoreEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CoreEngine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stateChanged((*reinterpret_cast<std::add_pointer_t<EngineState>>(_a[1]))); break;
        case 1: _t->channelChanged((*reinterpret_cast<std::add_pointer_t<ChannelInfo>>(_a[1]))); break;
        case 2: _t->signalLevelChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->epgUpdated(); break;
        case 5: _t->onNetworkStreamReady((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->onNetworkError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->updateStatus(); break;
        case 8: _t->onMediaPlayerStateChanged((*reinterpret_cast<std::add_pointer_t<QMediaPlayer::PlaybackState>>(_a[1]))); break;
        case 9: _t->onMediaPlayerError((*reinterpret_cast<std::add_pointer_t<QMediaPlayer::Error>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CoreEngine::*)(EngineState )>(_a, &CoreEngine::stateChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CoreEngine::*)(const ChannelInfo & )>(_a, &CoreEngine::channelChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CoreEngine::*)(int )>(_a, &CoreEngine::signalLevelChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CoreEngine::*)(const QString & )>(_a, &CoreEngine::errorOccurred, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CoreEngine::*)()>(_a, &CoreEngine::epgUpdated, 4))
            return;
    }
}

const QMetaObject *CoreEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CoreEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10CoreEngineE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CoreEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void CoreEngine::stateChanged(EngineState _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void CoreEngine::channelChanged(const ChannelInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void CoreEngine::signalLevelChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void CoreEngine::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void CoreEngine::epgUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
