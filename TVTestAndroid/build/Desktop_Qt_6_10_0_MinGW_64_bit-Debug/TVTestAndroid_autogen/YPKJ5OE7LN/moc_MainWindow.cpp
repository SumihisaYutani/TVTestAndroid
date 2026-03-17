/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/ui/MainWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "onConnectClicked",
        "",
        "onDisconnectClicked",
        "onSelectBonDriverClicked",
        "onSetChannelClicked",
        "onStartReceivingClicked",
        "onStopReceivingClicked",
        "onStartDirectStreamClicked",
        "onStopDirectStreamClicked",
        "onClearLogClicked",
        "onNetworkConnected",
        "onNetworkDisconnected",
        "onTsDataReceived",
        "data",
        "onChannelChanged",
        "BonDriverNetwork::TuningSpace",
        "space",
        "uint32_t",
        "channel",
        "onSignalLevelChanged",
        "level",
        "onErrorOccurred",
        "error",
        "onUpdateStatsTimer",
        "onQuickChannelSelected",
        "onDirectStreamPlaybackStateChanged",
        "QMediaPlayer::PlaybackState",
        "state",
        "onDirectStreamMediaInfoChanged",
        "info",
        "onDirectStreamErrorOccurred",
        "onDirectStreamBufferStatusChanged",
        "bufferSize",
        "bufferStatus",
        "saveSettings",
        "restoreSettings"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onConnectClicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDisconnectClicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSelectBonDriverClicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSetChannelClicked'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStartReceivingClicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStopReceivingClicked'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStartDirectStreamClicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStopDirectStreamClicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onClearLogClicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNetworkConnected'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNetworkDisconnected'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTsDataReceived'
        QtMocHelpers::SlotData<void(const QByteArray &)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QByteArray, 14 },
        }}),
        // Slot 'onChannelChanged'
        QtMocHelpers::SlotData<void(BonDriverNetwork::TuningSpace, uint32_t)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 16, 17 }, { 0x80000000 | 18, 19 },
        }}),
        // Slot 'onSignalLevelChanged'
        QtMocHelpers::SlotData<void(float)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Float, 21 },
        }}),
        // Slot 'onErrorOccurred'
        QtMocHelpers::SlotData<void(const QString &)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 23 },
        }}),
        // Slot 'onUpdateStatsTimer'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onQuickChannelSelected'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDirectStreamPlaybackStateChanged'
        QtMocHelpers::SlotData<void(QMediaPlayer::PlaybackState)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 27, 28 },
        }}),
        // Slot 'onDirectStreamMediaInfoChanged'
        QtMocHelpers::SlotData<void(const QString &)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 30 },
        }}),
        // Slot 'onDirectStreamErrorOccurred'
        QtMocHelpers::SlotData<void(const QString &)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 23 },
        }}),
        // Slot 'onDirectStreamBufferStatusChanged'
        QtMocHelpers::SlotData<void(qint64, int)>(32, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 33 }, { QMetaType::Int, 34 },
        }}),
        // Slot 'saveSettings'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'restoreSettings'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onConnectClicked(); break;
        case 1: _t->onDisconnectClicked(); break;
        case 2: _t->onSelectBonDriverClicked(); break;
        case 3: _t->onSetChannelClicked(); break;
        case 4: _t->onStartReceivingClicked(); break;
        case 5: _t->onStopReceivingClicked(); break;
        case 6: _t->onStartDirectStreamClicked(); break;
        case 7: _t->onStopDirectStreamClicked(); break;
        case 8: _t->onClearLogClicked(); break;
        case 9: _t->onNetworkConnected(); break;
        case 10: _t->onNetworkDisconnected(); break;
        case 11: _t->onTsDataReceived((*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[1]))); break;
        case 12: _t->onChannelChanged((*reinterpret_cast<std::add_pointer_t<BonDriverNetwork::TuningSpace>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[2]))); break;
        case 13: _t->onSignalLevelChanged((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 14: _t->onErrorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->onUpdateStatsTimer(); break;
        case 16: _t->onQuickChannelSelected(); break;
        case 17: _t->onDirectStreamPlaybackStateChanged((*reinterpret_cast<std::add_pointer_t<QMediaPlayer::PlaybackState>>(_a[1]))); break;
        case 18: _t->onDirectStreamMediaInfoChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->onDirectStreamErrorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->onDirectStreamBufferStatusChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 21: _t->saveSettings(); break;
        case 22: _t->restoreSettings(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 23;
    }
    return _id;
}
QT_WARNING_POP
