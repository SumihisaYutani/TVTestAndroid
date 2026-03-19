/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/MainWindow.h"
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
        "onClearLogClicked",
        "onQuickChannelSelected",
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
        "onUpdateStatsTimer"
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
        // Slot 'onClearLogClicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onQuickChannelSelected'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNetworkConnected'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNetworkDisconnected'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTsDataReceived'
        QtMocHelpers::SlotData<void(const QByteArray &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QByteArray, 13 },
        }}),
        // Slot 'onChannelChanged'
        QtMocHelpers::SlotData<void(BonDriverNetwork::TuningSpace, uint32_t)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 16 }, { 0x80000000 | 17, 18 },
        }}),
        // Slot 'onSignalLevelChanged'
        QtMocHelpers::SlotData<void(float)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Float, 20 },
        }}),
        // Slot 'onErrorOccurred'
        QtMocHelpers::SlotData<void(const QString &)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 22 },
        }}),
        // Slot 'onUpdateStatsTimer'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
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
        case 6: _t->onClearLogClicked(); break;
        case 7: _t->onQuickChannelSelected(); break;
        case 8: _t->onNetworkConnected(); break;
        case 9: _t->onNetworkDisconnected(); break;
        case 10: _t->onTsDataReceived((*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[1]))); break;
        case 11: _t->onChannelChanged((*reinterpret_cast<std::add_pointer_t<BonDriverNetwork::TuningSpace>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[2]))); break;
        case 12: _t->onSignalLevelChanged((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 13: _t->onErrorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->onUpdateStatsTimer(); break;
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
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}
QT_WARNING_POP
