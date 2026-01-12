#ifndef NUSBLECONNECTION_H
#define NUSBLECONNECTION_H

#include "MeshCoreConnection.h"
#ifdef Q_OS_LINUX
#include "BluezAgent.h"
#endif
#ifdef Q_OS_WIN
#include "WinRtBlePairing.h"
#endif
#include <QBluetoothDeviceInfo>
#include <QBluetoothLocalDevice>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyConnectionParameters>
#include <QTimer>
#include <QQueue>

namespace MeshCore {

/**
 * @brief Modern Qt 6.10+ BLE connection for MeshCore devices using Nordic UART Service
 *
 * This implementation properly handles:
 * - Nordic UART Service (NUS) protocol
 * - BLE-specific framing (no serial frame headers)
 * - MTU negotiation and chunked writes
 * - Notification setup with proper error recovery
 * - Connection parameter optimization for throughput
 *
 * Note: BLE does NOT use the 0x3c frame header that Serial uses.
 * Commands are sent raw, and responses come as raw notifications.
 */
class NusBleConnection : public MeshCoreConnection
{
    Q_OBJECT
    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled NOTIFY notificationsEnabledChanged)
    Q_PROPERTY(int mtu READ mtu NOTIFY mtuChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QString deviceAddress READ deviceAddress NOTIFY deviceAddressChanged)

public:
    explicit NusBleConnection(QObject *parent = nullptr);
    ~NusBleConnection() override;

    /**
     * @brief Connect to a BLE device
     * @param deviceInfo The BLE device to connect to
     */
    void connectToDevice(const QBluetoothDeviceInfo &deviceInfo);

    void close() override;

    /**
     * @brief Set the PIN code for BLE pairing
     * @param pin The PIN code (default: 123456)
     */
    void setPin(quint32 pin);
    quint32 pin() const;

    // Properties
    [[nodiscard]] bool notificationsEnabled() const { return m_notificationsEnabled; }
    [[nodiscard]] int mtu() const { return m_mtu; }
    [[nodiscard]] QString deviceName() const { return m_deviceName; }
    [[nodiscard]] QString deviceAddress() const { return m_deviceAddress; }

Q_SIGNALS:
    void notificationsEnabledChanged();
    void mtuChanged();
    void deviceNameChanged();
    void deviceAddressChanged();

protected:
    /**
     * @brief Send a frame to the device
     * 
     * For BLE, this sends the raw command data without any framing header.
     * The frame is chunked according to the negotiated MTU.
     */
    void sendToRadioFrame(const QByteArray &frame) override;

private Q_SLOTS:
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onControllerErrorOccurred(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid &serviceUuid);
    void onServiceDiscoveryFinished();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onServiceErrorOccurred(QLowEnergyService::ServiceError error);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onDescriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &value);
    void onMtuChanged(int mtu);
    void onPairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing);
    void onPairingError(QBluetoothLocalDevice::Error error);

private:
    void setupService();
    void setupServiceFromCache();  // Use characteristics without full discovery
    void enableNotifications();
    void requestPairing();  // Request pairing with the device
    void processWriteQueue();
    void writeChunk(const QByteArray &data);
    void startPolling();  // Fallback for Windows when notifications fail
    void pollCharacteristic();  // Poll TX characteristic for data

    // Controller and service
    std::unique_ptr<QLowEnergyController> m_controller;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyCharacteristic m_rxCharacteristic;  // We write to this (NUS RX)
    QLowEnergyCharacteristic m_txCharacteristic;  // We read from this (NUS TX)

    // Device info
    QString m_deviceName;
    QString m_deviceAddress;

    // Connection state
    bool m_notificationsEnabled = false;
    int m_mtu = 20;  // Default BLE MTU (conservative)
    bool m_writePending = false;

    // Write queue for chunked writes
    QQueue<QByteArray> m_writeQueue;
    QTimer *m_writeTimer = nullptr;

    // Retry handling
    int m_retryCount = 0;
    static constexpr int MaxRetries = 3;
    QTimer *m_retryTimer = nullptr;
    
    // Notification subscription retry (Windows)
    int m_notificationRetryCount = 0;
    static constexpr int MaxNotificationRetries = 3;
    
    // Polling fallback for Windows
    QTimer *m_pollTimer = nullptr;
    bool m_pollingEnabled = false;
    
    // Local device for pairing operations
    QBluetoothLocalDevice *m_localDevice = nullptr;
    QBluetoothAddress m_remoteAddress;
    bool m_pairingRequested = false;
    
#ifdef Q_OS_LINUX
    // BlueZ agent for PIN pairing (Linux only)
    BluezAgent *m_agent = nullptr;
#endif

#ifdef Q_OS_WIN
    // WinRT pairing helper (Windows only)
    WinRtBlePairing *m_winrtPairing = nullptr;
#endif

    // PIN code for pairing
    quint32 m_pin = 123456;
};

} // namespace MeshCore

#endif // NUSBLECONNECTION_H
