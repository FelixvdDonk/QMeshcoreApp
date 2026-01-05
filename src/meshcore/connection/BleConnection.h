#ifndef BLECONNECTION_H
#define BLECONNECTION_H

#include "MeshCoreConnection.h"
#include "BluezAgent.h"
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QTimer>

namespace MeshCore {

/**
 * @brief BLE connection to a MeshCore device
 *
 * Uses QLowEnergyController for BLE communication.
 * 
 * Note: Qt's BLE layer uses BlueZ on Linux. Some devices have issues with
 * CCCD writes for enabling notifications, which can cause disconnection.
 * If you experience connection issues, try DBusBleConnection instead.
 */
class BleConnection : public MeshCoreConnection
{
    Q_OBJECT
    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled NOTIFY notificationsEnabledChanged)
    Q_PROPERTY(bool skipNotifications READ skipNotifications WRITE setSkipNotifications NOTIFY skipNotificationsChanged)

public:
    explicit BleConnection(QObject *parent = nullptr);
    ~BleConnection() override;

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
    
    bool notificationsEnabled() const { return m_notificationsEnabled; }
    bool skipNotifications() const { return m_skipNotifications; }
    void setSkipNotifications(bool skip);

Q_SIGNALS:
    void notificationsEnabledChanged();
    void skipNotificationsChanged();

protected:
    void sendToRadioFrame(const QByteArray &frame) override;

private Q_SLOTS:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerErrorOccurred(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid &serviceUuid);
    void onServiceDiscoveryFinished();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    void setupService();
    void setupServiceDirect();
    void writeToDevice(const QByteArray &data);
    void retryServiceDiscovery();

    std::unique_ptr<QLowEnergyController> m_controller;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyCharacteristic m_rxCharacteristic;  // We write to this
    QLowEnergyCharacteristic m_txCharacteristic;  // We read from this
    QBluetoothDeviceInfo m_deviceInfo;

    int m_discoveryRetryCount = 0;
    static constexpr int MaxDiscoveryRetries = 3;
    QTimer *m_discoveryTimer = nullptr;
    
    bool m_notificationsEnabled = false;
    bool m_skipNotifications = false;  // Set true to avoid CCCD writes that cause disconnection
    
    // BlueZ agent for PIN pairing (Linux only)
    BluezAgent *m_agent = nullptr;
};

} // namespace MeshCore

#endif // BLECONNECTION_H
