#include "qmeshcore_plugin.h"

#include <QQmlEngine>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>

void qml_register_types_QMeshCore()
{
    // Register the singleton - QML engine takes ownership
    qmlRegisterSingletonType<MeshCore::MeshCoreDevice>(
        "QMeshCore", 1, 0, "MeshCoreDevice",
        [](QQmlEngine *engine, QJSEngine *) -> QObject * {
            auto *device = new MeshCore::MeshCoreDevice();
            // QML engine takes ownership of singleton objects automatically
            QQmlEngine::setObjectOwnership(device, QQmlEngine::CppOwnership);
            return device;
        });

    // Note: Q_GADGET value types (Contact, SelfInfo, etc.) don't need explicit
    // registration - they work automatically as property return types since
    // they have Q_GADGET and their properties are exposed via Q_PROPERTY.

    // Register uncreatable models (created and owned by MeshCoreDevice)
    qmlRegisterUncreatableType<MeshCore::ContactModel>(
        "QMeshCore", 1, 0, "ContactModel",
        "ContactModel is obtained from MeshCoreDevice");
    qmlRegisterUncreatableType<MeshCore::ChannelModel>(
        "QMeshCore", 1, 0, "ChannelModel",
        "ChannelModel is obtained from MeshCoreDevice");
    qmlRegisterUncreatableType<MeshCore::MessageModel>(
        "QMeshCore", 1, 0, "MessageModel",
        "MessageModel is obtained from MeshCoreDevice");
}
