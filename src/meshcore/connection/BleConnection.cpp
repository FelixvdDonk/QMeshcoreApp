#include "BleConnection.h"
#ifdef Q_OS_LINUX
#include "BluezAgent.h"
#endif
#include "../MeshCoreConstants.h"
#include <QDebug>
#include <QTimer>

namespace MeshCore {

BleConnection::BleConnection(QObject *parent)
    : MeshCoreConnection(parent)
{
#ifdef Q_OS_LINUX
    // Create and register the BlueZ agent for PIN pairing (Linux only)
    m_agent = new BluezAgent(this);
    if (!m_agent->registerAgent()) {
        qWarning() << "BLE: Failed to register BlueZ agent - pairing may not work automatically";
    }
#endif
}

BleConnection::~BleConnection()
{
    close();
#ifdef Q_OS_LINUX
    // Unregister agent explicitly before destruction
    if (m_agent) {
        m_agent->unregisterAgent();
    }
#endif
}

void BleConnection::setSkipNotifications(bool skip)
{
    if (m_skipNotifications != skip) {
        m_skipNotifications = skip;
        Q_EMIT skipNotificationsChanged();
    }
}

void BleConnection::setPin(quint32 pin)
{
#ifdef Q_OS_LINUX
    if (m_agent) {
        m_agent->setPin(pin);
        qDebug() << "BLE: PIN set to" << pin;
    }
#else
    Q_UNUSED(pin)
    qDebug() << "BLE: PIN setting not supported on this platform";
#endif
}

quint32 BleConnection::pin() const
{
#ifdef Q_OS_LINUX
    return m_agent ? m_agent->pin() : 123456;
#else
    return 123456;
#endif
}

void BleConnection::connectToDevice(const QBluetoothDeviceInfo &deviceInfo)
{
    if (m_controller) {
        close();
    }

    m_deviceInfo = deviceInfo;
    m_discoveryRetryCount = 0;

    // Create controller using factory method
    m_controller.reset(QLowEnergyController::createCentral(deviceInfo, this));
    
    // Explicitly set address type to work around Qt/bluez issues with random addresses
    // MeshCore devices typically use random addresses
    m_controller->setRemoteAddressType(QLowEnergyController::RandomAddress);

    // Connect signals
    connect(m_controller.get(), &QLowEnergyController::connected,
            this, &BleConnection::onControllerConnected);
    connect(m_controller.get(), &QLowEnergyController::disconnected,
            this, &BleConnection::onControllerDisconnected);
    connect(m_controller.get(), &QLowEnergyController::errorOccurred,
            this, &BleConnection::onControllerErrorOccurred);
    connect(m_controller.get(), &QLowEnergyController::serviceDiscovered,
            this, &BleConnection::onServiceDiscovered);
    connect(m_controller.get(), &QLowEnergyController::discoveryFinished,
            this, &BleConnection::onServiceDiscoveryFinished);

    // Start connection
    m_controller->connectToDevice();
}

void BleConnection::close()
{
    if (m_discoveryTimer) {
        m_discoveryTimer->stop();
        m_discoveryTimer->deleteLater();
        m_discoveryTimer = nullptr;
    }

    if (m_service) {
        m_service->deleteLater();
        m_service = nullptr;
    }

    if (m_controller) {
        m_controller->disconnectFromDevice();
        m_controller.reset();
    }

    m_rxCharacteristic = QLowEnergyCharacteristic();
    m_txCharacteristic = QLowEnergyCharacteristic();
    m_notificationsEnabled = false;
}

void BleConnection::onControllerConnected()
{
    qDebug() << "BLE connected, discovering services...";
    m_controller->discoverServices();
}

void BleConnection::onControllerDisconnected()
{
    qDebug() << "BLE disconnected";
    onDisconnected();
}

void BleConnection::onControllerErrorOccurred(QLowEnergyController::Error error)
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
        errorStr = QStringLiteral("Connection error");
        break;
    case QLowEnergyController::AdvertisingError:
        errorStr = QStringLiteral("Advertising error");
        break;
    case QLowEnergyController::RemoteHostClosedError:
        errorStr = QStringLiteral("Remote host closed connection");
        break;
    case QLowEnergyController::AuthorizationError:
        errorStr = QStringLiteral("Authorization error");
        break;
    case QLowEnergyController::MissingPermissionsError:
        errorStr = QStringLiteral("Missing permissions");
        break;
    case QLowEnergyController::RssiReadError:
        errorStr = QStringLiteral("RSSI read error");
        break;
    }

    Q_EMIT errorOccurred(errorStr);
}

void BleConnection::onServiceDiscovered(const QBluetoothUuid &serviceUuid)
{
    qDebug() << "Service discovered:" << serviceUuid.toString();
}

void BleConnection::onServiceDiscoveryFinished()
{
    qDebug() << "Service discovery finished";

    // Find the MeshCore service (Nordic UART)
    m_service = m_controller->createServiceObject(Ble::ServiceUuid, this);

    if (!m_service) {
        Q_EMIT errorOccurred(QStringLiteral("MeshCore service not found"));
        return;
    }

    connect(m_service, &QLowEnergyService::stateChanged,
            this, &BleConnection::onServiceStateChanged);
    connect(m_service, &QLowEnergyService::characteristicChanged,
            this, &BleConnection::onCharacteristicChanged);
    connect(m_service, &QLowEnergyService::errorOccurred,
            this, [this](QLowEnergyService::ServiceError error) {
                QString errorStr;
                switch (error) {
                case QLowEnergyService::NoError: errorStr = "NoError"; break;
                case QLowEnergyService::OperationError: errorStr = "OperationError"; break;
                case QLowEnergyService::CharacteristicWriteError: errorStr = "CharacteristicWriteError"; break;
                case QLowEnergyService::DescriptorWriteError: errorStr = "DescriptorWriteError"; break;
                case QLowEnergyService::UnknownError: errorStr = "UnknownError"; break;
                case QLowEnergyService::CharacteristicReadError: errorStr = "CharacteristicReadError"; break;
                case QLowEnergyService::DescriptorReadError: errorStr = "DescriptorReadError"; break;
                }
                qDebug() << "Service error:" << errorStr;
                // Don't emit error for descriptor read errors - they're often harmless
                if (error != QLowEnergyService::DescriptorReadError) {
                    Q_EMIT errorOccurred(QStringLiteral("Service error: %1").arg(errorStr));
                }
            });
    connect(m_service, &QLowEnergyService::characteristicWritten,
            this, [](const QLowEnergyCharacteristic &c, const QByteArray &value) {
                qDebug() << "Characteristic written:" << c.uuid() << value.size() << "bytes";
            });
    connect(m_service, &QLowEnergyService::descriptorWritten,
            this, [this](const QLowEnergyDescriptor &d, const QByteArray &value) {
                qDebug() << "Descriptor written:" << d.uuid() << value.toHex();
                // Notifications enabled, we're ready
                if (d.isValid() && value == QLowEnergyCharacteristic::CCCDEnableNotification) {
                    qDebug() << "Notifications enabled, connection ready";
                    m_notificationsEnabled = true;
                    Q_EMIT notificationsEnabledChanged();
                    if (!m_connected) {
                        onConnected();
                    }
                }
            });

    qDebug() << "Attempting to skip detailed discovery and connect directly...";
    
    // Try to use service without detailed discovery
    // This works when device is already bonded and GATT is cached
    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    connect(m_discoveryTimer, &QTimer::timeout, this, [this]() {
        if (m_service && !m_connected) {
            qDebug() << "Timeout - checking if we can use cached characteristics...";
            setupServiceDirect();
        }
    });
    
    // First try the normal discovery
    connect(m_service, &QLowEnergyService::stateChanged, this, [this](QLowEnergyService::ServiceState state) {
        if (state == QLowEnergyService::RemoteServiceDiscovered) {
            m_discoveryTimer->stop();
            setupService();
        }
    }, Qt::SingleShotConnection);
    
    m_service->discoverDetails();
    m_discoveryTimer->start(2000);  // 2 second timeout
}

void BleConnection::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    QString stateStr;
    switch (state) {
    case QLowEnergyService::InvalidService: stateStr = "InvalidService"; break;
    case QLowEnergyService::RemoteService: stateStr = "RemoteService"; break;
    case QLowEnergyService::RemoteServiceDiscovering: stateStr = "RemoteServiceDiscovering"; break;
    case QLowEnergyService::RemoteServiceDiscovered: stateStr = "RemoteServiceDiscovered"; break;
    case QLowEnergyService::LocalService: stateStr = "LocalService"; break;
    }
    qDebug() << "Service state changed:" << stateStr;
    
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        if (m_discoveryTimer) {
            m_discoveryTimer->stop();
        }
        setupService();
    } else if (state == QLowEnergyService::InvalidService) {
        // Service discovery failed - but if we're already connected, ignore it
        if (m_connected) {
            qDebug() << "Service went invalid but we're already connected, ignoring";
            return;
        }
        // Try to retry a few times
        if (m_discoveryRetryCount < MaxDiscoveryRetries) {
            qDebug() << "Service discovery failed, retrying..." << (m_discoveryRetryCount + 1) << "/" << MaxDiscoveryRetries;
            QTimer::singleShot(500, this, &BleConnection::retryServiceDiscovery);
        } else {
            qDebug() << "Service discovery failed after" << MaxDiscoveryRetries << "retries";
            Q_EMIT errorOccurred(QStringLiteral("Service discovery failed. The device may require pairing - try pairing in your system Bluetooth settings first."));
        }
    }
}

void BleConnection::retryServiceDiscovery()
{
    if (!m_controller) return;
    
    m_discoveryRetryCount++;
    
    // Clean up old service
    if (m_service) {
        m_service->deleteLater();
        m_service = nullptr;
    }
    
    // Recreate service object and try again
    m_service = m_controller->createServiceObject(Ble::ServiceUuid, this);
    if (!m_service) {
        Q_EMIT errorOccurred(QStringLiteral("MeshCore service not found on retry"));
        return;
    }
    
    connect(m_service, &QLowEnergyService::stateChanged,
            this, &BleConnection::onServiceStateChanged);
    connect(m_service, &QLowEnergyService::characteristicChanged,
            this, &BleConnection::onCharacteristicChanged);
    connect(m_service, &QLowEnergyService::errorOccurred,
            this, [this](QLowEnergyService::ServiceError error) {
                qDebug() << "Service error:" << error;
            });
    connect(m_service, &QLowEnergyService::characteristicWritten,
            this, [](const QLowEnergyCharacteristic &c, const QByteArray &value) {
                qDebug() << "Characteristic written:" << c.uuid() << value.size() << "bytes";
            });
    connect(m_service, &QLowEnergyService::descriptorWritten,
            this, [this](const QLowEnergyDescriptor &d, const QByteArray &value) {
                qDebug() << "Descriptor written:" << d.uuid() << value.toHex();
                if (d.isValid() && value == QLowEnergyCharacteristic::CCCDEnableNotification) {
                    qDebug() << "Notifications enabled, connection ready";
                    onConnected();
                }
            });
    
    qDebug() << "Retrying service detail discovery...";
    m_service->discoverDetails();
}

void BleConnection::setupService()
{
    qDebug() << "Setting up service, listing all characteristics:";
    for (const auto &c : m_service->characteristics()) {
        qDebug() << "  Characteristic:" << c.uuid().toString() 
                 << "properties:" << c.properties()
                 << "descriptors:" << c.descriptors().size();
        for (const auto &d : c.descriptors()) {
            qDebug() << "    Descriptor:" << d.uuid().toString();
        }
    }
    
    // Find RX characteristic (we write to this)
    m_rxCharacteristic = m_service->characteristic(Ble::CharacteristicUuidRx);
    if (!m_rxCharacteristic.isValid()) {
        qDebug() << "RX characteristic not found! UUID:" << Ble::CharacteristicUuidRx.toString();
        Q_EMIT errorOccurred(QStringLiteral("RX characteristic not found"));
        return;
    }
    qDebug() << "Found RX characteristic";

    // Find TX characteristic (we read from this)
    m_txCharacteristic = m_service->characteristic(Ble::CharacteristicUuidTx);
    if (!m_txCharacteristic.isValid()) {
        qDebug() << "TX characteristic not found! UUID:" << Ble::CharacteristicUuidTx.toString();
        Q_EMIT errorOccurred(QStringLiteral("TX characteristic not found"));
        return;
    }
    qDebug() << "Found TX characteristic";

    // Skip notifications if requested (some devices disconnect when enabling CCCD)
    if (m_skipNotifications) {
        qDebug() << "Skipping notification setup (skipNotifications=true)";
        qDebug() << "NOTE: Responses from device will not be received!";
        onConnected();
        return;
    }

    // Enable notifications on TX characteristic
    QLowEnergyDescriptor notificationDesc = m_txCharacteristic.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

    if (notificationDesc.isValid() && m_service->state() == QLowEnergyService::RemoteServiceDiscovered) {
        qDebug() << "Enabling notifications on TX characteristic...";
        m_service->writeDescriptor(notificationDesc, QLowEnergyCharacteristic::CCCDEnableNotification);
        // onConnected() will be called from descriptorWritten callback
    } else if (notificationDesc.isValid()) {
        qDebug() << "Service not fully discovered, trying to enable notifications anyway...";
        m_service->writeDescriptor(notificationDesc, QLowEnergyCharacteristic::CCCDEnableNotification);
        // Also connect after a delay in case the write fails
        QTimer::singleShot(1000, this, [this]() {
            if (!m_connected) {
                qDebug() << "Notification enable may have failed, connecting anyway...";
                onConnected();
            }
        });
    } else {
        qDebug() << "No CCCD descriptor found, connecting anyway...";
        onConnected();
    }

    qDebug() << "BLE service setup complete";
}

void BleConnection::setupServiceDirect()
{
    qDebug() << "Setting up service directly (skipping detailed discovery)";
    
    // Get characteristics - they should be available even without full discovery
    m_rxCharacteristic = m_service->characteristic(Ble::CharacteristicUuidRx);
    m_txCharacteristic = m_service->characteristic(Ble::CharacteristicUuidTx);
    
    if (!m_rxCharacteristic.isValid()) {
        qDebug() << "RX characteristic not available yet";
    } else {
        qDebug() << "Found RX characteristic";
    }
    
    if (!m_txCharacteristic.isValid()) {
        qDebug() << "TX characteristic not available yet";
    } else {
        qDebug() << "Found TX characteristic";
    }
    
    // Connect anyway - notifications might be enabled from previous session
    // or we can work without them for writes
    qDebug() << "Connecting without full service discovery...";
    onConnected();
}

void BleConnection::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic,
                                             const QByteArray &value)
{
    if (characteristic.uuid() == Ble::CharacteristicUuidTx) {
        onFrameReceived(value);
    }
}

void BleConnection::sendToRadioFrame(const QByteArray &frame)
{
    Q_EMIT frameSent(frame);
    writeToDevice(frame);
}

void BleConnection::writeToDevice(const QByteArray &data)
{
    if (!m_service || !m_rxCharacteristic.isValid()) {
        qWarning() << "Cannot write: service not ready";
        return;
    }

    qDebug() << "BLE writing" << data.size() << "bytes";
    
    // Just try to write - ignore service state
    // Qt's state machine is broken for some devices
    m_service->writeCharacteristic(m_rxCharacteristic, data, QLowEnergyService::WriteWithoutResponse);
}

} // namespace MeshCore
