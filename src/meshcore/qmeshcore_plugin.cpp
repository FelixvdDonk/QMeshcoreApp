#include "qmeshcore_plugin.h"
#include "MeshCoreDeviceController.h"

#include <QQmlEngine>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>

void qml_register_types_QMeshCore()
{
    // Register the controller singleton - this is the main QML interface
    // MeshCoreDevice runs on a worker thread, controller is on main thread
    qmlRegisterSingletonType<MeshCore::MeshCoreDeviceController>(
        "QMeshCore", 1, 0, "MeshCoreDevice",
        [](QQmlEngine *engine, QJSEngine *) -> QObject * {
            auto *controller = new MeshCore::MeshCoreDeviceController();
            // QML engine takes ownership of singleton objects automatically
            QQmlEngine::setObjectOwnership(controller, QQmlEngine::CppOwnership);
            return controller;
        });

    // Note: Q_GADGET value types (Contact, SelfInfo, etc.) don't need explicit
    // registration - they work automatically as property return types since
    // they have Q_GADGET and their properties are exposed via Q_PROPERTY.

    // Register uncreatable models (created and owned by MeshCoreDeviceController)
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

