#ifndef BLUEZAGENT_H
#define BLUEZAGENT_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

namespace MeshCore {

/**
 * @brief BlueZ Agent for handling BLE pairing with PIN
 *
 * Implements org.bluez.Agent1 interface to automatically respond
 * to pairing requests with the configured PIN (default: 123456).
 *
 * BlueZ Agent methods:
 * - RequestPasskey: Returns numeric passkey for pairing
 * - RequestPinCode: Returns string PIN code
 * - DisplayPasskey: Called when device displays a passkey
 * - RequestConfirmation: Called for numeric comparison
 * - AuthorizeService: Called to authorize a service
 * - Cancel: Called when pairing is cancelled
 * - Release: Called when agent is unregistered
 */
class BluezAgent : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.bluez.Agent1")

public:
    explicit BluezAgent(QObject *parent = nullptr);
    ~BluezAgent();

    /**
     * @brief Register the agent with BlueZ
     * @return true if registration successful
     */
    bool registerAgent();

    /**
     * @brief Unregister the agent from BlueZ
     */
    void unregisterAgent();

    /**
     * @brief Set the PIN code for pairing
     * @param pin The PIN code (default: 123456)
     */
    void setPin(quint32 pin) { m_pin = pin; }
    quint32 pin() const { return m_pin; }

    /**
     * @brief Check if agent is registered
     */
    bool isRegistered() const { return m_registered; }

    /**
     * @brief Get the agent object path
     */
    static QString agentPath() { return QStringLiteral("/org/qmeshcore/agent"); }

Q_SIGNALS:
    void pairingRequested(const QString &devicePath);
    void pairingComplete(const QString &devicePath, bool success);

public Q_SLOTS:
    // org.bluez.Agent1 interface methods (exposed via D-Bus)
    
    /**
     * @brief Called when device requests a passkey
     * @param device D-Bus path of the device
     * @return The numeric passkey (PIN)
     */
    quint32 RequestPasskey(const QDBusObjectPath &device);

    /**
     * @brief Called when device requests a PIN code (string)
     * @param device D-Bus path of the device
     * @return The PIN code as string
     */
    QString RequestPinCode(const QDBusObjectPath &device);

    /**
     * @brief Called when device displays a passkey
     * @param device D-Bus path of the device
     * @param passkey The passkey being displayed
     * @param entered Number of digits entered
     */
    void DisplayPasskey(const QDBusObjectPath &device, quint32 passkey, quint16 entered);

    /**
     * @brief Called for numeric comparison confirmation
     * @param device D-Bus path of the device
     * @param passkey The passkey to confirm
     */
    void RequestConfirmation(const QDBusObjectPath &device, quint32 passkey);

    /**
     * @brief Called to authorize a service
     * @param device D-Bus path of the device
     * @param uuid The service UUID
     */
    void AuthorizeService(const QDBusObjectPath &device, const QString &uuid);

    /**
     * @brief Called when pairing is cancelled
     */
    void Cancel();

    /**
     * @brief Called when agent is released
     */
    void Release();

private:
    quint32 m_pin = 123456;  // Default MeshCore PIN
    bool m_registered = false;
};

} // namespace MeshCore

#endif // BLUEZAGENT_H
