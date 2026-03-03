/****************************************************************************
** Meta object code from reading C++ file 'EPGDatabase.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/database/EPGDatabase.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EPGDatabase.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11EPGDatabaseE_t {};
} // unnamed namespace

template <> constexpr inline auto EPGDatabase::qt_create_metaobjectdata<qt_meta_tag_ZN11EPGDatabaseE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "EPGDatabase",
        "eventAdded",
        "",
        "EPGEvent",
        "event",
        "eventUpdated",
        "eventRemoved",
        "eventId",
        "serviceAdded",
        "ServiceInfo",
        "service",
        "serviceUpdated",
        "databaseError",
        "error",
        "performMaintenance",
        "cleanupExpiredEvents"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'eventAdded'
        QtMocHelpers::SignalData<void(const EPGEvent &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'eventUpdated'
        QtMocHelpers::SignalData<void(const EPGEvent &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'eventRemoved'
        QtMocHelpers::SignalData<void(int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 },
        }}),
        // Signal 'serviceAdded'
        QtMocHelpers::SignalData<void(const ServiceInfo &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Signal 'serviceUpdated'
        QtMocHelpers::SignalData<void(const ServiceInfo &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Signal 'databaseError'
        QtMocHelpers::SignalData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Slot 'performMaintenance'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'cleanupExpiredEvents'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<EPGDatabase, qt_meta_tag_ZN11EPGDatabaseE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject EPGDatabase::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11EPGDatabaseE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11EPGDatabaseE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11EPGDatabaseE_t>.metaTypes,
    nullptr
} };

void EPGDatabase::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<EPGDatabase *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->eventAdded((*reinterpret_cast<std::add_pointer_t<EPGEvent>>(_a[1]))); break;
        case 1: _t->eventUpdated((*reinterpret_cast<std::add_pointer_t<EPGEvent>>(_a[1]))); break;
        case 2: _t->eventRemoved((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->serviceAdded((*reinterpret_cast<std::add_pointer_t<ServiceInfo>>(_a[1]))); break;
        case 4: _t->serviceUpdated((*reinterpret_cast<std::add_pointer_t<ServiceInfo>>(_a[1]))); break;
        case 5: _t->databaseError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->performMaintenance(); break;
        case 7: _t->cleanupExpiredEvents(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (EPGDatabase::*)(const EPGEvent & )>(_a, &EPGDatabase::eventAdded, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (EPGDatabase::*)(const EPGEvent & )>(_a, &EPGDatabase::eventUpdated, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (EPGDatabase::*)(int )>(_a, &EPGDatabase::eventRemoved, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (EPGDatabase::*)(const ServiceInfo & )>(_a, &EPGDatabase::serviceAdded, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (EPGDatabase::*)(const ServiceInfo & )>(_a, &EPGDatabase::serviceUpdated, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (EPGDatabase::*)(const QString & )>(_a, &EPGDatabase::databaseError, 5))
            return;
    }
}

const QMetaObject *EPGDatabase::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EPGDatabase::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11EPGDatabaseE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EPGDatabase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void EPGDatabase::eventAdded(const EPGEvent & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void EPGDatabase::eventUpdated(const EPGEvent & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void EPGDatabase::eventRemoved(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void EPGDatabase::serviceAdded(const ServiceInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void EPGDatabase::serviceUpdated(const ServiceInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void EPGDatabase::databaseError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}
QT_WARNING_POP
