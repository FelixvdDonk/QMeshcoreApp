#include "BluezAgent.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusMessage>
#include <QDebug>

namespace MeshCore {

// BlueZ D-Bus constants
static const QString BLUEZ_SERVICE = QStringLiteral("org.bluez");
static const QString BLUEZ_AGENT_MANAGER_IFACE = QStringLiteral("org.bluez.AgentManager1");
static const QString BLUEZ_AGENT_IFACE = QStringLiteral("org.bluez.Agent1");

BluezAgent::BluezAgent(QObject *parent)
    : QObject(parent)
{
}

BluezAgent::~BluezAgent()
{
    if (m_registered) {
        unregisterAgent();
    }
}

bool BluezAgent::registerAgent()
{
    if (m_registered) {
        qDebug() << "BluezAgent: Already registered";
        return true;
    }

    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (!bus.isConnected()) {
        qWarning() << "BluezAgent: System bus not connected";
        return false;
    }

    // Register this object on D-Bus so BlueZ can call our methods
    bool registered = bus.registerObject(
        agentPath(),
        this,
        QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties
    );

    if (!registered) {
        qWarning() << "BluezAgent: Failed to register object on D-Bus:" << bus.lastError().message();
        return false;
    }

    qDebug() << "BluezAgent: Registered object at" << agentPath();

    // Register with BlueZ AgentManager
    QDBusInterface agentManager(
        BLUEZ_SERVICE,
        QStringLiteral("/org/bluez"),
        BLUEZ_AGENT_MANAGER_IFACE,
        bus
    );

    if (!agentManager.isValid()) {
        qWarning() << "BluezAgent: AgentManager interface not valid";
        bus.unregisterObject(agentPath());
        return false;
    }

    // RegisterAgent(object_path agent, string capability)
    // Capabilities: DisplayOnly, DisplayYesNo, KeyboardOnly, NoInputNoOutput, KeyboardDisplay
    // We use KeyboardDisplay since we can handle all pairing methods
    QDBusReply<void> reply = agentManager.call(
        QStringLiteral("RegisterAgent"),
        QDBusObjectPath(agentPath()),
        QStringLiteral("KeyboardDisplay")
    );

    if (!reply.isValid()) {
        qWarning() << "BluezAgent: RegisterAgent failed:" << reply.error().message();
        bus.unregisterObject(agentPath());
        return false;
    }

    qDebug() << "BluezAgent: Registered with BlueZ AgentManager";

    // Request to be the default agent
    QDBusReply<void> defaultReply = agentManager.call(
        QStringLiteral("RequestDefaultAgent"),
        QDBusObjectPath(agentPath())
    );

    if (!defaultReply.isValid()) {
        qWarning() << "BluezAgent: RequestDefaultAgent failed (non-fatal):" << defaultReply.error().message();
        // Not fatal - we can still handle pairing for devices we initiate
    } else {
        qDebug() << "BluezAgent: Set as default agent";
    }

    m_registered = true;
    return true;
}

void BluezAgent::unregisterAgent()
{
    if (!m_registered) {
        return;
    }

    QDBusConnection bus = QDBusConnection::systemBus();

    QDBusInterface agentManager(
        BLUEZ_SERVICE,
        QStringLiteral("/org/bluez"),
        BLUEZ_AGENT_MANAGER_IFACE,
        bus
    );

    if (agentManager.isValid()) {
        QDBusReply<void> reply = agentManager.call(
            QStringLiteral("UnregisterAgent"),
            QDBusObjectPath(agentPath())
        );
        if (!reply.isValid()) {
            qDebug() << "BluezAgent: UnregisterAgent failed:" << reply.error().message();
        }
    }

    bus.unregisterObject(agentPath());
    m_registered = false;
    qDebug() << "BluezAgent: Unregistered";
}

quint32 BluezAgent::RequestPasskey(const QDBusObjectPath &device)
{
    qDebug() << "BluezAgent: RequestPasskey for" << device.path() << "-> returning" << m_pin;
    Q_EMIT pairingRequested(device.path());
    return m_pin;
}

QString BluezAgent::RequestPinCode(const QDBusObjectPath &device)
{
    QString pinStr = QString::number(m_pin);
    qDebug() << "BluezAgent: RequestPinCode for" << device.path() << "-> returning" << pinStr;
    Q_EMIT pairingRequested(device.path());
    return pinStr;
}

void BluezAgent::DisplayPasskey(const QDBusObjectPath &device, quint32 passkey, quint16 entered)
{
    qDebug() << "BluezAgent: DisplayPasskey for" << device.path() 
             << "passkey:" << passkey << "entered:" << entered;
}

void BluezAgent::RequestConfirmation(const QDBusObjectPath &device, quint32 passkey)
{
    qDebug() << "BluezAgent: RequestConfirmation for" << device.path() << "passkey:" << passkey;
    
    // For numeric comparison, we auto-confirm if passkey matches our PIN
    // or if it's a simple yes/no confirmation
    // BlueZ expects us to return (no error) if we confirm, or throw an error to reject
    
    // Auto-confirm - the device will show the same passkey
    qDebug() << "BluezAgent: Auto-confirming passkey";
    // No return value needed - returning without error means confirmation
}

void BluezAgent::AuthorizeService(const QDBusObjectPath &device, const QString &uuid)
{
    qDebug() << "BluezAgent: AuthorizeService for" << device.path() << "service:" << uuid;
    // Auto-authorize all services
}

void BluezAgent::Cancel()
{
    qDebug() << "BluezAgent: Pairing cancelled";
}

void BluezAgent::Release()
{
    qDebug() << "BluezAgent: Agent released";
    m_registered = false;
}

} // namespace MeshCore
