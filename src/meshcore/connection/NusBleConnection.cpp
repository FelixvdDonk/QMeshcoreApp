#include "NusBleConnection.h"
#ifdef Q_OS_LINUX
#include "BluezAgent.h"
#endif
#ifdef Q_OS_WIN
#include "WinRtBlePairing.h"
#endif
#include "../MeshCoreConstants.h"
#include <QDebug>
#include <QTimer>

namespace MeshCore {

NusBleConnection::NusBleConnection(QObject *parent)
    : MeshCoreConnection(parent)
{
#ifdef Q_OS_LINUX
    // Create and register the BlueZ agent for PIN pairing (Linux only)
    m_agent = new BluezAgent(this);
    if (!m_agent->registerAgent()) {
        qWarning() << "NUS BLE: Failed to register BlueZ agent - pairing may not work automatically";
    }
#endif

#ifdef Q_OS_WIN
    // Create WinRT pairing helper for PIN pairing (Windows only)
    m_winrtPairing = new WinRtBlePairing(this);
    connect(m_winrtPairing, &WinRtBlePairing::pairingProgress, this, [this](const QString &status) {
        qDebug() << "NUS BLE: WinRT pairing:" << status;
    });
    connect(m_winrtPairing, &WinRtBlePairing::pairingFinished, this, 
            [this](const QBluetoothAddress &address, bool success, const QString &error) {
        if (address != m_remoteAddress) return;
        
        m_pairingRequested = false;
        if (success) {
            qDebug() << "NUS BLE: WinRT pairing successful! Retrying notification setup...";
            m_notificationRetryCount = 0;
            if (m_txCharacteristic.isValid()) {
                enableNotifications();
            }
        } else {
            qWarning() << "NUS BLE: WinRT pairing failed:" << error;
            Q_EMIT errorOccurred(QStringLiteral("Pairing failed: %1").arg(error));
        }
    });
#endif

    // Timer for processing write queue with delay between chunks
    m_writeTimer = new QTimer(this);
    m_writeTimer->setSingleShot(true);
    m_writeTimer->setInterval(15);  // 15ms between BLE writes
    connect(m_writeTimer, &QTimer::timeout, this, &NusBleConnection::processWriteQueue);

    // Retry timer
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    
    // Poll timer for Windows fallback when notifications don't work
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(100);  // Poll every 100ms
    connect(m_pollTimer, &QTimer::timeout, this, &NusBleConnection::pollCharacteristic);
    
    // Create local device for pairing operations
    m_localDevice = new QBluetoothLocalDevice(this);
    connect(m_localDevice, &QBluetoothLocalDevice::pairingFinished,
            this, &NusBleConnection::onPairingFinished);
    connect(m_localDevice, &QBluetoothLocalDevice::errorOccurred,
            this, &NusBleConnection::onPairingError);
}

NusBleConnection::~NusBleConnection()
{
    close();
#ifdef Q_OS_LINUX
    // Unregister agent explicitly before destruction
    if (m_agent) {
        m_agent->unregisterAgent();
    }
#endif
}

void NusBleConnection::connectToDevice(const QBluetoothDeviceInfo &deviceInfo)
{
    if (m_controller) {
        close();
    }

    m_deviceName = deviceInfo.name();
    m_deviceAddress = deviceInfo.address().toString();
    m_remoteAddress = deviceInfo.address();  // Store for pairing
    m_retryCount = 0;
    m_notificationRetryCount = 0;
    m_notificationsEnabled = false;
    m_pairingRequested = false;
    m_mtu = 20;  // Reset to default

    qDebug() << "NUS BLE: Connecting to" << m_deviceName << m_deviceAddress;

    Q_EMIT deviceNameChanged();
    Q_EMIT deviceAddressChanged();

    // Create controller using Qt 6 factory method
    m_controller.reset(QLowEnergyController::createCentral(deviceInfo, this));

#ifdef Q_OS_WIN
    // On Windows, try to determine address type from device info
    // If the device uses a random address (starts with certain patterns), set RandomAddress
    // Otherwise use PublicAddress which works better for some devices
    QBluetoothAddress addr = deviceInfo.address();
    QString addrStr = addr.toString();
    // Random addresses typically have the two MSBs of the first byte set
    // (address type is in the first byte, e.g., C0:xx or similar)
    bool isRandom = false;
    if (!addrStr.isEmpty()) {
        bool ok;
        int firstByte = addrStr.left(2).toInt(&ok, 16);
        // Random addresses: 0x40-0x7F (non-resolvable) or 0xC0-0xFF (static random)
        isRandom = ok && ((firstByte >= 0x40 && firstByte <= 0x7F) || (firstByte >= 0xC0));
    }
    if (isRandom) {
        qDebug() << "NUS BLE: Using Random address type (detected from address)";
        m_controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
    } else {
        qDebug() << "NUS BLE: Using Public address type";
        m_controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    }
#else
    // MeshCore devices typically use random addresses
    m_controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
#endif

    // Connect to controller signals
    connect(m_controller.get(), &QLowEnergyController::stateChanged,
            this, &NusBleConnection::onControllerStateChanged);
    connect(m_controller.get(), &QLowEnergyController::errorOccurred,
            this, &NusBleConnection::onControllerErrorOccurred);
    connect(m_controller.get(), &QLowEnergyController::serviceDiscovered,
            this, &NusBleConnection::onServiceDiscovered);
    connect(m_controller.get(), &QLowEnergyController::discoveryFinished,
            this, &NusBleConnection::onServiceDiscoveryFinished);
    connect(m_controller.get(), &QLowEnergyController::mtuChanged,
            this, &NusBleConnection::onMtuChanged);

    // Start connection
    m_controller->connectToDevice();
}

void NusBleConnection::close()
{
    qDebug() << "NUS BLE: Closing connection";

    m_writeTimer->stop();
    m_retryTimer->stop();
    m_pollTimer->stop();
    m_pollingEnabled = false;
    m_writeQueue.clear();
    m_writePending = false;

    if (m_service) {
        // Try to stop notifications gracefully
        if (m_txCharacteristic.isValid()) {
            QLowEnergyDescriptor cccd = m_txCharacteristic.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (cccd.isValid()) {
                // Disable notifications by writing 0x0000
                m_service->writeDescriptor(cccd, QByteArray::fromHex("0000"));
            }
        }
        m_service->deleteLater();
        m_service = nullptr;
    }

    if (m_controller) {
        if (m_controller->state() != QLowEnergyController::UnconnectedState) {
            m_controller->disconnectFromDevice();
        }
        m_controller.reset();
    }

    m_rxCharacteristic = QLowEnergyCharacteristic();
    m_txCharacteristic = QLowEnergyCharacteristic();
    m_notificationsEnabled = false;

    if (m_connected) {
        onDisconnected();
    }
}

void NusBleConnection::setPin(quint32 pin)
{
    m_pin = pin;
#ifdef Q_OS_LINUX
    if (m_agent) {
        m_agent->setPin(pin);
        qDebug() << "NUS BLE: PIN set to" << pin;
    }
#else
    qDebug() << "NUS BLE: PIN set to" << pin;
#endif
}

quint32 NusBleConnection::pin() const
{
    return m_pin;
}

void NusBleConnection::onControllerStateChanged(QLowEnergyController::ControllerState state)
{
    qDebug() << "NUS BLE: Controller state changed:" << state;

    switch (state) {
    case QLowEnergyController::UnconnectedState:
        if (m_connected) {
            qDebug() << "NUS BLE: Disconnected";
            onDisconnected();
        }
        break;

    case QLowEnergyController::ConnectedState:
        qDebug() << "NUS BLE: Connected, discovering services...";
        m_controller->discoverServices();
        break;

    case QLowEnergyController::DiscoveringState:
        qDebug() << "NUS BLE: Discovering services...";
        break;

    case QLowEnergyController::DiscoveredState:
        qDebug() << "NUS BLE: Services discovered";
        break;

    default:
        break;
    }
}

void NusBleConnection::onControllerErrorOccurred(QLowEnergyController::Error error)
{
    QString errorStr;
    switch (error) {
    case QLowEnergyController::NoError:
        return;
    case QLowEnergyController::UnknownError:
        errorStr = QStringLiteral("Unknown error");
        break;
    case QLowEnergyController::UnknownRemoteDeviceError:
        errorStr = QStringLiteral("Unknown remote device");
        break;
    case QLowEnergyController::NetworkError:
        errorStr = QStringLiteral("Network error");
        break;
    case QLowEnergyController::InvalidBluetoothAdapterError:
        errorStr = QStringLiteral("Invalid Bluetooth adapter");
        break;
    case QLowEnergyController::ConnectionError:
        errorStr = QStringLiteral("Connection error - device may need pairing");
#ifdef Q_OS_WIN
        // On Windows, try requesting pairing on connection error
        if (!m_pairingRequested) {
            qDebug() << "NUS BLE: Connection error, attempting to pair device...";
            requestPairing();
        }
#endif
        break;
    case QLowEnergyController::AdvertisingError:
        errorStr = QStringLiteral("Advertising error");
        break;
    case QLowEnergyController::RemoteHostClosedError:
        errorStr = QStringLiteral("Device closed connection");
        break;
    case QLowEnergyController::AuthorizationError:
        errorStr = QStringLiteral("Authorization error - pairing required");
#ifdef Q_OS_WIN
        // On Windows, AuthorizationError means we need to pair
        if (!m_pairingRequested) {
            qDebug() << "NUS BLE: Authorization error, device requires pairing";
            requestPairing();
            return;  // Don't emit error yet, wait for pairing result
        }
#endif
        break;
    case QLowEnergyController::MissingPermissionsError:
        errorStr = QStringLiteral("Missing Bluetooth permissions");
        break;
    case QLowEnergyController::RssiReadError:
        errorStr = QStringLiteral("RSSI read error");
        break;
    }

    qWarning() << "NUS BLE: Controller error:" << errorStr;
    Q_EMIT errorOccurred(errorStr);
}

void NusBleConnection::onServiceDiscovered(const QBluetoothUuid &serviceUuid)
{
    qDebug() << "NUS BLE: Service discovered:" << serviceUuid.toString();
}

void NusBleConnection::onServiceDiscoveryFinished()
{
    qDebug() << "NUS BLE: Service discovery finished";

    // Find the Nordic UART Service
    m_service = m_controller->createServiceObject(Ble::ServiceUuid, this);

    if (!m_service) {
        qWarning() << "NUS BLE: Nordic UART Service not found!";
        qWarning() << "NUS BLE: Available services:";
        for (const auto &uuid : m_controller->services()) {
            qWarning() << "  -" << uuid.toString();
        }
        Q_EMIT errorOccurred(QStringLiteral("MeshCore service not found. Is this a MeshCore device?"));
        return;
    }

    qDebug() << "NUS BLE: Found Nordic UART Service";

    // Connect service signals
    connect(m_service, &QLowEnergyService::stateChanged,
            this, &NusBleConnection::onServiceStateChanged);
    connect(m_service, &QLowEnergyService::errorOccurred,
            this, &NusBleConnection::onServiceErrorOccurred);
    connect(m_service, &QLowEnergyService::characteristicChanged,
            this, &NusBleConnection::onCharacteristicChanged);
    connect(m_service, &QLowEnergyService::characteristicWritten,
            this, &NusBleConnection::onCharacteristicWritten);
    connect(m_service, &QLowEnergyService::characteristicRead,
            this, &NusBleConnection::onCharacteristicRead);
    connect(m_service, &QLowEnergyService::descriptorWritten,
            this, &NusBleConnection::onDescriptorWritten);

    // Call discoverDetails() - Qt needs this to populate characteristics.
    // It may fail to read some descriptors on encrypted devices, but that's OK -
    // we'll use the characteristics anyway once they're discovered.
    qDebug() << "NUS BLE: Starting characteristic discovery...";
    
    // Set a timeout - if discovery doesn't complete cleanly, try to use whatever we have
    QTimer::singleShot(5000, this, [this]() {
        if (!m_connected && m_service) {
            qDebug() << "NUS BLE: Discovery timeout - trying to use available characteristics";
            auto chars = m_service->characteristics();
            qDebug() << "NUS BLE: Found" << chars.size() << "characteristics after timeout";
            if (chars.size() >= 2) {
                setupService();
            } else {
                Q_EMIT errorOccurred(QStringLiteral("Service discovery timed out"));
            }
        }
    });
    
    m_service->discoverDetails();
}

void NusBleConnection::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    qDebug() << "NUS BLE: Service state changed:" << state;

    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        setupService();
    } else if (state == QLowEnergyService::InvalidService) {
        // Service discovery failed - this often happens on Linux when the device
        // requires bonding/encryption. Try to emit a helpful error message.
        if (!m_connected) {
            qWarning() << "NUS BLE: Service discovery failed - device may require pairing";
            qWarning() << "NUS BLE: Try pairing the device first using your system Bluetooth settings";
            qWarning() << "NUS BLE: Or use 'bluetoothctl pair" << m_deviceAddress << "' from terminal";
            Q_EMIT errorOccurred(QStringLiteral("Service discovery failed. Please pair the device first using system Bluetooth settings, then try again."));
        }
    }
}

void NusBleConnection::onServiceErrorOccurred(QLowEnergyService::ServiceError error)
{
    QString errorStr;
    switch (error) {
    case QLowEnergyService::NoError:
        return;
    case QLowEnergyService::OperationError:
        errorStr = QStringLiteral("Operation error");
        // If we're not connected yet, this might be from CCCD write - try connecting anyway
        if (!m_connected && m_rxCharacteristic.isValid()) {
            qDebug() << "NUS BLE: Operation error but characteristics valid, connecting anyway";
            onConnected();
            return;
        }
        break;
    case QLowEnergyService::CharacteristicWriteError:
        errorStr = QStringLiteral("Write error");
        m_writePending = false;  // Allow retrying
        break;
    case QLowEnergyService::DescriptorWriteError:
        errorStr = QStringLiteral("Descriptor write error (notification setup failed)");
#ifdef Q_OS_WIN
        // On Windows, retry CCCD write a few times with delay
        if (m_notificationRetryCount < MaxNotificationRetries && !m_connected) {
            m_notificationRetryCount++;
            qDebug() << "NUS BLE: CCCD write failed, retrying..." << m_notificationRetryCount << "/" << MaxNotificationRetries;
            QTimer::singleShot(500, this, [this]() {
                if (!m_notificationsEnabled && m_txCharacteristic.isValid()) {
                    enableNotifications();
                }
            });
            return;
        }
        // If retries exhausted, try requesting pairing
        if (!m_connected && !m_pairingRequested) {
            qDebug() << "NUS BLE: CCCD write failed after retries, attempting to pair device...";
            qWarning() << "NUS BLE: *** NOTIFICATION SETUP FAILED - ATTEMPTING PAIRING ***";
            qWarning() << "NUS BLE: If a Windows pairing dialog appears, enter PIN: 123456";
            
            // Try to request pairing programmatically
            requestPairing();
            
            // Also try connecting anyway in case pairing isn't needed
            QTimer::singleShot(5000, this, [this]() {
                if (!m_connected && !m_pairingRequested && m_rxCharacteristic.isValid()) {
                    qDebug() << "NUS BLE: Pairing timeout, connecting anyway";
                    onConnected();
                }
            });
        }
        return;
#else
        // Try connecting anyway - notifications might already be enabled
        if (!m_connected) {
            qDebug() << "NUS BLE: CCCD write failed, trying to connect anyway";
            onConnected();
        }
        return;  // Don't emit error for this
#endif
    case QLowEnergyService::CharacteristicReadError:
        errorStr = QStringLiteral("Read error");
        break;
    case QLowEnergyService::DescriptorReadError:
        // This is the "NotPermitted" error from encrypted devices.
        // The characteristics should still be discoverable, just ignore this error.
        qDebug() << "NUS BLE: Descriptor read error (expected on encrypted devices) - ignoring";
        return;  // Don't emit error, don't do anything - wait for discovery to complete
    case QLowEnergyService::UnknownError:
    default:
        errorStr = QStringLiteral("Unknown service error");
        break;
    }

    qWarning() << "NUS BLE: Service error:" << errorStr;
    // Only emit for serious errors
    if (error == QLowEnergyService::CharacteristicWriteError) {
        Q_EMIT errorOccurred(errorStr);
    }
}

void NusBleConnection::setupService()
{
    qDebug() << "NUS BLE: Setting up service characteristics";

    // List all characteristics for debugging
    for (const auto &c : m_service->characteristics()) {
        qDebug() << "  Characteristic:" << c.uuid().toString()
                 << "properties:" << c.properties();
    }

    // Find RX characteristic (we write to this - device receives)
    m_rxCharacteristic = m_service->characteristic(Ble::CharacteristicUuidRx);
    if (!m_rxCharacteristic.isValid()) {
        qWarning() << "NUS BLE: RX characteristic not found!";
        Q_EMIT errorOccurred(QStringLiteral("RX characteristic not found"));
        return;
    }
    qDebug() << "NUS BLE: Found RX characteristic (write)";

    // Find TX characteristic (we read from this via notifications - device sends)
    m_txCharacteristic = m_service->characteristic(Ble::CharacteristicUuidTx);
    if (!m_txCharacteristic.isValid()) {
        qWarning() << "NUS BLE: TX characteristic not found!";
        Q_EMIT errorOccurred(QStringLiteral("TX characteristic not found"));
        return;
    }
    qDebug() << "NUS BLE: Found TX characteristic (notify)";

    // Enable notifications
    enableNotifications();
}

void NusBleConnection::setupServiceFromCache()
{
    // This is called when discoverDetails() fails (e.g., descriptor read not permitted).
    // Try to use characteristics anyway - they may be cached by BlueZ from previous connections.
    qDebug() << "NUS BLE: Trying to set up service from cache (bypassing full discovery)";

    if (!m_service) {
        Q_EMIT errorOccurred(QStringLiteral("Service not available"));
        return;
    }

    // Get characteristics - they may be available even without full discovery
    auto characteristics = m_service->characteristics();
    qDebug() << "NUS BLE: Found" << characteristics.size() << "characteristics in cache";

    for (const auto &c : characteristics) {
        qDebug() << "  Cached characteristic:" << c.uuid().toString();
        if (c.uuid() == Ble::CharacteristicUuidRx) {
            m_rxCharacteristic = c;
            qDebug() << "NUS BLE: Found RX characteristic from cache";
        } else if (c.uuid() == Ble::CharacteristicUuidTx) {
            m_txCharacteristic = c;
            qDebug() << "NUS BLE: Found TX characteristic from cache";
        }
    }

    if (m_rxCharacteristic.isValid() && m_txCharacteristic.isValid()) {
        qDebug() << "NUS BLE: Using cached characteristics";
        enableNotifications();
    } else {
        // Characteristics not cached - we really need to pair the device
        qWarning() << "NUS BLE: Characteristics not available in cache";
        qWarning() << "NUS BLE: The device likely requires pairing. Use 'bluetoothctl pair" << m_deviceAddress << "'";
        Q_EMIT errorOccurred(QStringLiteral("Service discovery failed. Please pair the device using system Bluetooth settings first."));
    }
}

void NusBleConnection::enableNotifications()
{
    qDebug() << "NUS BLE: Enabling notifications on TX characteristic";

#ifdef Q_OS_WIN
    // On Windows, Qt 6's BLE implementation may handle notifications differently.
    // Try multiple approaches:
    
    // First, check if the characteristic supports notifications
    if (!(m_txCharacteristic.properties() & QLowEnergyCharacteristic::Notify)) {
        qWarning() << "NUS BLE: TX characteristic doesn't support Notify property!";
    }
    
    // List all descriptors for debugging
    qDebug() << "NUS BLE: TX characteristic has" << m_txCharacteristic.descriptors().size() << "descriptors";
    for (const auto &desc : m_txCharacteristic.descriptors()) {
        qDebug() << "  Descriptor:" << desc.uuid().toString() 
                 << "valid:" << desc.isValid()
                 << "value:" << desc.value().toHex();
    }
#endif

    QLowEnergyDescriptor cccd = m_txCharacteristic.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

    if (!cccd.isValid()) {
        qWarning() << "NUS BLE: CCCD descriptor not found in cache";
#ifdef Q_OS_WIN
        // On Windows, try to find CCCD by iterating descriptors
        QBluetoothUuid cccdUuid(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        for (const auto &desc : m_txCharacteristic.descriptors()) {
            if (desc.uuid() == cccdUuid) {
                cccd = desc;
                qDebug() << "NUS BLE: Found CCCD by iteration!";
                break;
            }
        }
        
        if (!cccd.isValid()) {
            // CCCD not found - on Windows 10/11, try connecting anyway
            // The Windows BLE stack sometimes handles subscriptions automatically
            // when characteristicChanged signal is connected
            qWarning() << "NUS BLE: CCCD not found - Windows may handle notifications automatically";
            qDebug() << "NUS BLE: Connecting anyway and hoping characteristicChanged works...";
            m_notificationsEnabled = true;  // Assume it might work
            Q_EMIT notificationsEnabledChanged();
            onConnected();
            return;
        }
#else
        // Some devices work without explicit CCCD write
        onConnected();
        return;
#endif
    }

    qDebug() << "NUS BLE: Writing CCCD to enable notifications, descriptor valid:" << cccd.isValid();
    
    // Write to enable notifications - use the standard BLE notification enable value (0x0100)
    m_service->writeDescriptor(cccd, QLowEnergyCharacteristic::CCCDEnableNotification);

    // Set a timeout in case the CCCD write hangs
    QTimer::singleShot(3000, this, [this]() {
        if (!m_connected && m_rxCharacteristic.isValid()) {
            qDebug() << "NUS BLE: CCCD write timed out, connecting anyway";
            onConnected();
        }
    });
}

void NusBleConnection::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic,
                                                const QByteArray &value)
{
    if (characteristic.uuid() == Ble::CharacteristicUuidTx) {
        qDebug() << "NUS BLE: Received notification:" << value.size() << "bytes:" << value.toHex();
        // BLE receives raw protocol data - no frame header to strip
        onFrameReceived(value);
    }
}

void NusBleConnection::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic,
                                                const QByteArray &value)
{
    Q_UNUSED(characteristic)
    qDebug() << "NUS BLE: Write completed:" << value.size() << "bytes";
    m_writePending = false;

    // Process next chunk if any
    if (!m_writeQueue.isEmpty()) {
        m_writeTimer->start();
    }
}

void NusBleConnection::onCharacteristicRead(const QLowEnergyCharacteristic &characteristic,
                                             const QByteArray &value)
{
    // This is called when polling reads data from the TX characteristic
    if (characteristic.uuid() == Ble::CharacteristicUuidTx && !value.isEmpty()) {
        qDebug() << "NUS BLE: Read data from TX characteristic:" << value.size() << "bytes:" << value.toHex();
        onFrameReceived(value);
    }
}

void NusBleConnection::onDescriptorWritten(const QLowEnergyDescriptor &descriptor,
                                            const QByteArray &value)
{
    qDebug() << "NUS BLE: Descriptor written:" << descriptor.uuid().toString() << value.toHex();

    if (value == QLowEnergyCharacteristic::CCCDEnableNotification) {
        qDebug() << "NUS BLE: Notifications enabled successfully!";
        m_notificationsEnabled = true;
        Q_EMIT notificationsEnabledChanged();

        if (!m_connected) {
            onConnected();
        }
    }
}

void NusBleConnection::onMtuChanged(int mtu)
{
    qDebug() << "NUS BLE: MTU changed to" << mtu;
    // Account for ATT header overhead (3 bytes)
    m_mtu = qMax(20, mtu - 3);
    Q_EMIT mtuChanged();
}

void NusBleConnection::requestPairing()
{
    if (m_remoteAddress.isNull()) {
        qWarning() << "NUS BLE: Cannot request pairing - address not valid";
        return;
    }
    
    qDebug() << "NUS BLE: Requesting pairing with" << m_remoteAddress.toString();
    
#ifdef Q_OS_WIN
    // Use WinRT pairing with PIN on Windows
    if (m_winrtPairing) {
        // Check if already paired
        if (m_winrtPairing->isPaired(m_remoteAddress)) {
            qDebug() << "NUS BLE: Device already paired (WinRT)";
            if (!m_notificationsEnabled && m_txCharacteristic.isValid()) {
                enableNotifications();
            }
            return;
        }
        
        qDebug() << "NUS BLE: Initiating WinRT pairing with PIN" << m_pin;
        m_pairingRequested = true;
        m_winrtPairing->pairWithPin(m_remoteAddress, m_pin);
        return;
    }
#endif

    // Fallback: Use QBluetoothLocalDevice pairing
    if (m_localDevice) {
        // Check current pairing status
        QBluetoothLocalDevice::Pairing currentPairing = m_localDevice->pairingStatus(m_remoteAddress);
        qDebug() << "NUS BLE: Current pairing status:" << currentPairing;
        
        if (currentPairing == QBluetoothLocalDevice::Paired || 
            currentPairing == QBluetoothLocalDevice::AuthorizedPaired) {
            qDebug() << "NUS BLE: Device already paired";
            // Try enabling notifications again now that we know device is paired
            if (!m_notificationsEnabled && m_txCharacteristic.isValid()) {
                enableNotifications();
            }
            return;
        }
        
        // Request pairing - this should trigger Windows pairing dialog
        qDebug() << "NUS BLE: Initiating pairing request...";
        m_pairingRequested = true;
        m_localDevice->requestPairing(m_remoteAddress, QBluetoothLocalDevice::Paired);
    } else {
        qWarning() << "NUS BLE: Cannot request pairing - no pairing handler available";
    }
}

void NusBleConnection::onPairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing)
{
    if (address != m_remoteAddress) {
        return;  // Not our device
    }
    
    qDebug() << "NUS BLE: Pairing finished for" << address.toString() << "status:" << pairing;
    m_pairingRequested = false;
    
    if (pairing == QBluetoothLocalDevice::Paired || pairing == QBluetoothLocalDevice::AuthorizedPaired) {
        qDebug() << "NUS BLE: Pairing successful! Retrying notification setup...";
        // Reset retry counter and try again
        m_notificationRetryCount = 0;
        if (m_txCharacteristic.isValid()) {
            enableNotifications();
        }
    } else {
        qWarning() << "NUS BLE: Pairing failed or was rejected";
        Q_EMIT errorOccurred(QStringLiteral("Pairing failed. Please try again and enter PIN: 123456 when prompted."));
    }
}

void NusBleConnection::onPairingError(QBluetoothLocalDevice::Error error)
{
    qWarning() << "NUS BLE: Pairing error:" << error;
    m_pairingRequested = false;
    
    QString errorStr;
    switch (error) {
    case QBluetoothLocalDevice::PairingError:
        errorStr = QStringLiteral("Pairing failed");
        break;
    case QBluetoothLocalDevice::UnknownError:
    default:
        errorStr = QStringLiteral("Unknown pairing error");
        break;
    }
    
    Q_EMIT errorOccurred(errorStr + QStringLiteral(". PIN should be: 123456"));
}

void NusBleConnection::sendToRadioFrame(const QByteArray &frame)
{
    if (!m_service || !m_rxCharacteristic.isValid()) {
        qWarning() << "NUS BLE: Cannot send - not connected";
        return;
    }

    qDebug() << "NUS BLE: Sending frame:" << frame.size() << "bytes:" << frame.toHex();
    Q_EMIT frameSent(frame);

    // For BLE, we send raw data without the serial frame header
    // Split into MTU-sized chunks if needed
    int offset = 0;
    while (offset < frame.size()) {
        int chunkSize = qMin(m_mtu, static_cast<int>(frame.size() - offset));
        QByteArray chunk = frame.mid(offset, chunkSize);
        m_writeQueue.enqueue(chunk);
        offset += chunkSize;
    }

    // Start processing queue
    if (!m_writePending) {
        processWriteQueue();
    }
}

void NusBleConnection::processWriteQueue()
{
    if (m_writeQueue.isEmpty() || m_writePending) {
        return;
    }

    QByteArray chunk = m_writeQueue.dequeue();
    writeChunk(chunk);
}

void NusBleConnection::writeChunk(const QByteArray &data)
{
    if (!m_service || !m_rxCharacteristic.isValid()) {
        return;
    }

    m_writePending = true;

    // Determine write mode based on characteristic properties
    QLowEnergyService::WriteMode writeMode = QLowEnergyService::WriteWithResponse;
    if (m_rxCharacteristic.properties() & QLowEnergyCharacteristic::WriteNoResponse) {
        writeMode = QLowEnergyService::WriteWithoutResponse;
    }

    qDebug() << "NUS BLE: Writing chunk:" << data.size() << "bytes,"
             << (writeMode == QLowEnergyService::WriteWithoutResponse ? "no-response" : "with-response");

    m_service->writeCharacteristic(m_rxCharacteristic, data, writeMode);

    // For write-without-response, we won't get characteristicWritten signal
    if (writeMode == QLowEnergyService::WriteWithoutResponse) {
        m_writePending = false;
        if (!m_writeQueue.isEmpty()) {
            m_writeTimer->start();
        }
    }
}

void NusBleConnection::startPolling()
{
#ifdef Q_OS_WIN
    if (!m_pollingEnabled && m_txCharacteristic.isValid()) {
        qDebug() << "NUS BLE: Starting characteristic polling as fallback for notifications";
        m_pollingEnabled = true;
        m_pollTimer->start();
    }
#endif
}

void NusBleConnection::pollCharacteristic()
{
#ifdef Q_OS_WIN
    if (!m_service || !m_txCharacteristic.isValid() || !m_connected) {
        m_pollTimer->stop();
        m_pollingEnabled = false;
        return;
    }
    
    // Read the TX characteristic - the characteristicRead signal will be emitted
    if (m_txCharacteristic.properties() & QLowEnergyCharacteristic::Read) {
        m_service->readCharacteristic(m_txCharacteristic);
    }
#endif
}

} // namespace MeshCore
