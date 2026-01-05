#ifndef DBUSBLECONNECTION_H
#define DBUSBLECONNECTION_H

#include "MeshCoreConnection.h"
#include "BluezAgent.h"
#include <QBluetoothDeviceInfo>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QTimer>
#include <QSocketNotifier>

namespace MeshCore {

/**
 * @brief BLE connection using DBus directly to BlueZ
 *
 * This bypasses Qt's BLE layer which has issues with some devices.
 * It talks directly to BlueZ via DBus for GATT operations.
 *
 * Key findings from testing with MeshCore devices:
 * - AcquireWrite() returns a file descriptor for reliable writes
 * - StartNotify/AcquireNotify cause device disconnection on some devices
 * - WriteValue() may timeout due to BlueZ internal issues
 * - Works in "write-only" mode if notifications fail
 */
class DBusBleConnection : public MeshCoreConnection
{
    Q_OBJECT
    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled NOTIFY notificationsEnabledChanged)
    Q_PROPERTY(bool writeOnlyMode READ writeOnlyMode NOTIFY writeOnlyModeChanged)

public:
    explicit DBusBleConnection(QObject *parent = nullptr);
    ~DBusBleConnection() override;

    /**
     * @brief Connect to a BLE device
     * @param deviceInfo The BLE device to connect to
     */
    void connectToDevice(const QBluetoothDeviceInfo &deviceInfo);

    /**
     * @brief Set the PIN code for BLE pairing
     * @param pin The PIN code (default: 123456)
     */
    void setPin(quint32 pin);
    quint32 pin() const;

    /**
     * @brief Check if notifications are enabled
     */
    bool notificationsEnabled() const { return m_notificationsEnabled; }

    /**
     * @brief Check if running in write-only mode (no notifications)
     */
    bool writeOnlyMode() const { return m_writeOnlyMode; }

Q_SIGNALS:
    void notificationsEnabledChanged();
    void writeOnlyModeChanged();

public:
    void close() override;

protected:
    void sendToRadioFrame(const QByteArray &frame) override;

private Q_SLOTS:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);
    void onWriteFdReadyRead();
    void onNotifyFdReadyRead();

private:
    bool findDevicePath();
    bool connectDevice();
    bool ensurePaired();      // Ensure device is paired (uses agent if needed)
    bool findCharacteristics();
    bool acquireWrite();      // Use AcquireWrite for fd-based writing
    bool tryAcquireNotify();  // Try AcquireNotify (may fail on some devices)
    void subscribeToNotifications();  // Subscribe to property changes for notifications
    void checkNotificationsEnabled(); // Check if notifications are already on
    void tryStartNotifySync();    // Try StartNotify synchronously (may cause disconnection!)
    void tryStartNotify();    // Fallback: try StartNotify async (may cause disconnection!)
    void writeToDevice(const QByteArray &data);
    void writeViaFd(const QByteArray &data);
    void writeViaDBus(const QByteArray &data);

    QBluetoothDeviceInfo m_deviceInfo;
    QString m_deviceAddress;  // Stored separately for safe access
    QString m_devicePath;
    QString m_rxCharPath;  // We write to this (NUS RX)
    QString m_txCharPath;  // We read from this (NUS TX)
    
    QDBusInterface *m_deviceInterface = nullptr;
    QDBusInterface *m_rxCharInterface = nullptr;
    QDBusInterface *m_txCharInterface = nullptr;
    
    // File descriptors from AcquireWrite/AcquireNotify
    int m_writeFd = -1;
    int m_notifyFd = -1;
    uint16_t m_writeMtu = 20;  // Default BLE MTU
    uint16_t m_notifyMtu = 20;
    QSocketNotifier *m_notifyNotifier = nullptr;
    
    QTimer *m_connectionTimer = nullptr;
    int m_retryCount = 0;
    static constexpr int MaxRetries = 3;
    
    bool m_notificationsEnabled = false;
    bool m_writeOnlyMode = false;
    
    // BlueZ agent for PIN pairing
    BluezAgent *m_agent = nullptr;
};

} // namespace MeshCore

#endif // DBUSBLECONNECTION_H
