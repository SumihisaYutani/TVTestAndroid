/****************************************************************************
** Meta object code from reading C++ file 'TsStreamDevice.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/media/TsStreamDevice.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TsStreamDevice.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14TsStreamDeviceE_t {};
} // unnamed namespace

template <> constexpr inline auto TsStreamDevice::qt_create_metaobjectdata<qt_meta_tag_ZN14TsStreamDeviceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TsStreamDevice",
        "dataAvailable",
        "",
        "bufferSizeChanged",
        "size"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'dataAvailable'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'bufferSizeChanged'
        QtMocHelpers::SignalData<void(qint64)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::LongLong, 4 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TsStreamDevice, qt_meta_tag_ZN14TsStreamDeviceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TsStreamDevice::staticMetaObject = { {
    QMetaObject::SuperData::link<QIODevice::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TsStreamDeviceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TsStreamDeviceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14TsStreamDeviceE_t>.metaTypes,
    nullptr
} };

void TsStreamDevice::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TsStreamDevice *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->dataAvailable(); break;
        case 1: _t->bufferSizeChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TsStreamDevice::*)()>(_a, &TsStreamDevice::dataAvailable, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TsStreamDevice::*)(qint64 )>(_a, &TsStreamDevice::bufferSizeChanged, 1))
            return;
    }
}

const QMetaObject *TsStreamDevice::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TsStreamDevice::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TsStreamDeviceE_t>.strings))
        return static_cast<void*>(this);
    return QIODevice::qt_metacast(_clname);
}

int TsStreamDevice::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QIODevice::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void TsStreamDevice::dataAvailable()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TsStreamDevice::bufferSizeChanged(qint64 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
