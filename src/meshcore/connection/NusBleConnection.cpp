#include "NusBleConnection.h"
#include "BluezAgent.h"
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

    // Timer for processing write queue with delay between chunks
    m_writeTimer = new QTimer(this);
    m_writeTimer->setSingleShot(true);
    m_writeTimer->setInterval(15);  // 15ms between BLE writes
    connect(m_writeTimer, &QTimer::timeout, this, &NusBleConnection::processWriteQueue);

    // Retry timer
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
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
    m_retryCount = 0;
    m_notificationsEnabled = false;
    m_mtu = 20;  // Reset to default

    qDebug() << "NUS BLE: Connecting to" << m_deviceName << m_deviceAddress;

    Q_EMIT deviceNameChanged();
    Q_EMIT deviceAddressChanged();

    // Create controller using Qt 6 factory method
    m_controller.reset(QLowEnergyController::createCentral(deviceInfo, this));

    // MeshCore devices typically use random addresses
    m_controller->setRemoteAddressType(QLowEnergyController::RandomAddress);

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
#ifdef Q_OS_LINUX
    if (m_agent) {
        m_agent->setPin(pin);
        qDebug() << "NUS BLE: PIN set to" << pin;
    }
#else
    Q_UNUSED(pin)
    qDebug() << "NUS BLE: PIN setting not supported on this platform";
#endif
}

quint32 NusBleConnection::pin() const
{
#ifdef Q_OS_LINUX
    return m_agent ? m_agent->pin() : 123456;
#else
    return 123456;
#endif
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
        break;
    case QLowEnergyController::AdvertisingError:
        errorStr = QStringLiteral("Advertising error");
        break;
    case QLowEnergyController::RemoteHostClosedError:
        errorStr = QStringLiteral("Device closed connection");
        break;
    case QLowEnergyController::AuthorizationError:
        errorStr = QStringLiteral("Authorization error - try pairing the device first");
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
        // Try connecting anyway - notifications might already be enabled
        if (!m_connected) {
            qDebug() << "NUS BLE: CCCD write failed, trying to connect anyway";
            onConnected();
        }
        return;  // Don't emit error for this
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

    QLowEnergyDescriptor cccd = m_txCharacteristic.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

    if (!cccd.isValid()) {
        qWarning() << "NUS BLE: CCCD descriptor not found, connecting anyway";
        // Some devices work without explicit CCCD write
        onConnected();
        return;
    }

    // Write to enable notifications
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

} // namespace MeshCore
