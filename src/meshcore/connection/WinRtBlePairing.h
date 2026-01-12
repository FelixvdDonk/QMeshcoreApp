#ifndef WINRTBLEPAIRING_H
#define WINRTBLEPAIRING_H

#include <QObject>
#include <QString>
#include <QBluetoothAddress>

#ifdef Q_OS_WIN

namespace MeshCore {

/**
 * @brief Windows-specific BLE pairing helper using WinRT APIs
 * 
 * This class handles BLE device pairing on Windows, including PIN entry.
 * It uses the Windows Runtime (WinRT) DeviceInformation.Pairing APIs
 * which provide more control over the pairing process than Qt's 
 * QBluetoothLocalDevice.
 */
class WinRtBlePairing : public QObject
{
    Q_OBJECT

public:
    explicit WinRtBlePairing(QObject *parent = nullptr);
    ~WinRtBlePairing();

    /**
     * @brief Check if a device is paired
     * @param address Bluetooth address of the device
     * @return true if paired, false otherwise
     */
    bool isPaired(const QBluetoothAddress &address);

    /**
     * @brief Unpair a device
     * @param address Bluetooth address of the device
     * @return true if unpaired successfully
     */
    bool unpair(const QBluetoothAddress &address);

    /**
     * @brief Pair with a BLE device using PIN
     * @param address Bluetooth address of the device
     * @param pin PIN code to use (default 123456)
     * @return true if pairing initiated (result comes via signal)
     */
    bool pairWithPin(const QBluetoothAddress &address, quint32 pin = 123456);

    /**
     * @brief Set the PIN to use for pairing requests
     */
    void setPin(quint32 pin) { m_pin = pin; }
    quint32 pin() const { return m_pin; }

Q_SIGNALS:
    void pairingFinished(const QBluetoothAddress &address, bool success, const QString &errorMessage);
    void pairingProgress(const QString &message);

private:
    quint32 m_pin = 123456;
    
    // Private implementation to hide WinRT types from header
    class Private;
    Private *d = nullptr;
};

} // namespace MeshCore

#endif // Q_OS_WIN

#endif // WINRTBLEPAIRING_H
