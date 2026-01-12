#include "MeshCoreDevice.h"
#include "connection/BleConnection.h"
#include "connection/NusBleConnection.h"
#ifdef Q_OS_LINUX
#include "connection/DBusBleConnection.h"
#endif
#include "connection/SerialConnection.h"
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>

namespace MeshCore {

MeshCoreDevice::MeshCoreDevice(QObject *parent)
    : QObject(parent)
{
    // Initialize BLE discovery agent
    m_bleDiscoveryAgent = std::make_unique<QBluetoothDeviceDiscoveryAgent>(this);
    m_bleDiscoveryAgent->setLowEnergyDiscoveryTimeout(10000);  // 10 second timeout

    connect(m_bleDiscoveryAgent.get(), &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &MeshCoreDevice::onBleDeviceDiscovered);
    connect(m_bleDiscoveryAgent.get(), &QBluetoothDeviceDiscoveryAgent::finished,
            this, &MeshCoreDevice::onBleScanFinished);
    connect(m_bleDiscoveryAgent.get(), &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &MeshCoreDevice::onBleScanError);
}

MeshCoreDevice::~MeshCoreDevice()
{
    disconnect();
}

void MeshCoreDevice::setConnectionState(ConnectionState state)
{
    if (m_connectionState != state) {
        m_connectionState = state;
        Q_EMIT connectionStateChanged();
        Q_EMIT connectedChanged();
    }
}

void MeshCoreDevice::setErrorString(const QString &error)
{
    if (m_errorString != error) {
        m_errorString = error;
        Q_EMIT errorStringChanged();
    }
}

void MeshCoreDevice::setupConnectionSignals()
{
    if (!m_connection) {
        return;
    }

    // Connection state
    connect(m_connection.get(), &MeshCoreConnection::connected,
            this, &MeshCoreDevice::onConnectionConnected);
    connect(m_connection.get(), &MeshCoreConnection::disconnected,
            this, &MeshCoreDevice::onConnectionDisconnected);
    connect(m_connection.get(), &MeshCoreConnection::errorOccurred,
            this, &MeshCoreDevice::onConnectionError);

    // Responses
    connect(m_connection.get(), &MeshCoreConnection::selfInfoReceived,
            this, &MeshCoreDevice::onSelfInfoReceived);
    connect(m_connection.get(), &MeshCoreConnection::deviceInfoReceived,
            this, &MeshCoreDevice::onDeviceInfoReceived);
    connect(m_connection.get(), &MeshCoreConnection::contactsStarted,
            this, &MeshCoreDevice::onContactsStarted);
    connect(m_connection.get(), &MeshCoreConnection::contactReceived,
            this, &MeshCoreDevice::onContactReceived);
    connect(m_connection.get(), &MeshCoreConnection::contactsEnded,
            this, &MeshCoreDevice::onContactsEnded);
    connect(m_connection.get(), &MeshCoreConnection::channelInfoReceived,
            this, &MeshCoreDevice::onChannelInfoReceived);
    connect(m_connection.get(), &MeshCoreConnection::batteryVoltageReceived,
            this, &MeshCoreDevice::onBatteryVoltageReceived);
    connect(m_connection.get(), &MeshCoreConnection::sentResponse,
            this, &MeshCoreDevice::onSentResponse);
    connect(m_connection.get(), &MeshCoreConnection::contactMessageReceived,
            this, &MeshCoreDevice::onContactMsgReceived);
    connect(m_connection.get(), &MeshCoreConnection::channelMessageReceived,
            this, &MeshCoreDevice::onChannelMsgReceived);
    connect(m_connection.get(), &MeshCoreConnection::noMoreMessages,
            this, &MeshCoreDevice::onNoMoreMessages);
    connect(m_connection.get(), &MeshCoreConnection::exportContactReceived,
            this, &MeshCoreDevice::onExportContactReceived);

    // Push notifications
    connect(m_connection.get(), &MeshCoreConnection::newAdvertPush,
            this, &MeshCoreDevice::onNewAdvertPush);
    connect(m_connection.get(), &MeshCoreConnection::pathUpdatedPush,
            this, &MeshCoreDevice::onPathUpdatedPush);
    connect(m_connection.get(), &MeshCoreConnection::sendConfirmedPush,
            this, &MeshCoreDevice::onSendConfirmedPush);
    connect(m_connection.get(), &MeshCoreConnection::msgWaitingPush,
            this, &MeshCoreDevice::onMsgWaitingPush);
    connect(m_connection.get(), &MeshCoreConnection::statusResponsePush,
            this, &MeshCoreDevice::onStatusResponsePush);
    connect(m_connection.get(), &MeshCoreConnection::telemetryResponsePush,
            this, &MeshCoreDevice::onTelemetryResponsePush);
    connect(m_connection.get(), &MeshCoreConnection::traceDataPush,
            this, &MeshCoreDevice::onTraceDataPush);
    connect(m_connection.get(), &MeshCoreConnection::logRxDataPush,
            this, &MeshCoreDevice::onLogRxDataPush);
}

void MeshCoreDevice::cleanupConnection()
{
    if (m_connection) {
        m_connection->disconnect();
        m_connection.reset();
    }

    m_connectionType = ConnectionType::None;
    Q_EMIT connectionTypeChanged();

    // Clear state
    m_selfInfo = SelfInfo();
    m_deviceInfo = DeviceInfo();
    m_batteryMilliVolts = 0;
    m_contactModel.clear();
    m_channelModel.clear();

    Q_EMIT selfInfoChanged();
    Q_EMIT deviceInfoChanged();
    Q_EMIT batteryMilliVoltsChanged();
}

// BLE Scanning
void MeshCoreDevice::startBleScan()
{
    if (m_scanning) {
        return;
    }

    m_discoveredBleDevices.clear();
    m_discoveredBleDeviceInfos.clear();
    Q_EMIT discoveredBleDevicesChanged();

    m_bleDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    m_scanning = true;
    Q_EMIT scanningChanged();
}

void MeshCoreDevice::stopBleScan()
{
    if (m_scanning) {
        m_bleDiscoveryAgent->stop();
        m_scanning = false;
        Q_EMIT scanningChanged();
    }
}

void MeshCoreDevice::onBleDeviceDiscovered(const QBluetoothDeviceInfo &deviceInfo)
{
    // Filter for devices advertising the MeshCore service
    if (!deviceInfo.serviceUuids().contains(Ble::ServiceUuid)) {
        return;
    }

    // Check if already in list
    for (const auto &existing : m_discoveredBleDeviceInfos) {
        if (existing.address() == deviceInfo.address()) {
            return;
        }
    }

    m_discoveredBleDeviceInfos.append(deviceInfo);

    QVariantMap device;
    device[QStringLiteral("name")] = deviceInfo.name().isEmpty()
                                     ? QStringLiteral("Unknown Device")
                                     : deviceInfo.name();
    device[QStringLiteral("address")] = deviceInfo.address().toString();
    device[QStringLiteral("rssi")] = deviceInfo.rssi();
    m_discoveredBleDevices.append(device);
    Q_EMIT discoveredBleDevicesChanged();
}

void MeshCoreDevice::onBleScanFinished()
{
    m_scanning = false;
    Q_EMIT scanningChanged();
}

void MeshCoreDevice::onBleScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    Q_UNUSED(error)
    setErrorString(m_bleDiscoveryAgent->errorString());
    m_scanning = false;
    Q_EMIT scanningChanged();
}

void MeshCoreDevice::connectBle(int deviceIndex)
{
    if (deviceIndex < 0 || deviceIndex >= m_discoveredBleDeviceInfos.size()) {
        setErrorString(QStringLiteral("Invalid device index"));
        return;
    }

    // Stop scanning first - active scan can slow down connection
    stopBleScan();
    
    disconnect();  // Disconnect any existing connection

    setConnectionState(ConnectionState::Connecting);
    setErrorString(QString());

    // Platform-specific BLE connection:
    // - Linux: Use DBusBleConnection - Qt's BLE has issues with encrypted devices because it uses
    //   BlueZ's WriteValue instead of AcquireWrite. DBusBleConnection uses AcquireWrite directly.
    // - Other platforms (macOS, Windows, iOS, Android): Use NusBleConnection (Qt's QLowEnergyController)
#ifdef Q_OS_LINUX
    auto *bleConn = new DBusBleConnection(this);
#else
    auto *bleConn = new NusBleConnection(this);
#endif
    m_connection.reset(bleConn);
    m_connectionType = ConnectionType::Ble;
    Q_EMIT connectionTypeChanged();

    setupConnectionSignals();
    bleConn->connectToDevice(m_discoveredBleDeviceInfos.at(deviceIndex));
}

void MeshCoreDevice::connectBleByAddress(const QString &address)
{
    for (int i = 0; i < m_discoveredBleDeviceInfos.size(); ++i) {
        if (m_discoveredBleDeviceInfos.at(i).address().toString() == address) {
            connectBle(i);
            return;
        }
    }
    setErrorString(QStringLiteral("Device not found: %1").arg(address));
}

// Serial operations
QVariantList MeshCoreDevice::availableSerialPorts() const
{
    QVariantList ports;
    for (const auto &portInfo : QSerialPortInfo::availablePorts()) {
        QVariantMap port;
        port[QStringLiteral("name")] = portInfo.portName();
        port[QStringLiteral("systemLocation")] = portInfo.systemLocation();
        port[QStringLiteral("description")] = portInfo.description();
        port[QStringLiteral("manufacturer")] = portInfo.manufacturer();
        port[QStringLiteral("vendorId")] = portInfo.vendorIdentifier();
        port[QStringLiteral("productId")] = portInfo.productIdentifier();
        ports.append(port);
    }
    return ports;
}

void MeshCoreDevice::refreshSerialPorts()
{
    Q_EMIT availableSerialPortsChanged();
}

void MeshCoreDevice::connectSerial(const QString &portName, int baudRate)
{
    disconnect();

    setConnectionState(ConnectionState::Connecting);
    setErrorString(QString());

    auto *serialConn = new SerialConnection(this);
    m_connection.reset(serialConn);
    m_connectionType = ConnectionType::Serial;
    Q_EMIT connectionTypeChanged();

    setupConnectionSignals();
    serialConn->connectToPort(portName, baudRate);
}

void MeshCoreDevice::connectSerialByIndex(int portIndex, int baudRate)
{
    auto ports = QSerialPortInfo::availablePorts();
    if (portIndex < 0 || portIndex >= ports.size()) {
        setErrorString(QStringLiteral("Invalid port index"));
        return;
    }

    connectSerial(ports.at(portIndex).portName(), baudRate);
}

void MeshCoreDevice::disconnect()
{
    if (m_connection) {
        m_connection->close();
    }
    cleanupConnection();
    setConnectionState(ConnectionState::Disconnected);
}

// Connection events
void MeshCoreDevice::onConnectionConnected()
{
    qDebug() << "MeshCoreDevice: Connection connected";
    setConnectionState(ConnectionState::Connected);
    setErrorString(QString());
    // Note: MeshCoreConnection automatically sends DeviceQuery on connect
    // which returns device info. We don't send AppStart here to avoid duplicates.
}

void MeshCoreDevice::onConnectionDisconnected()
{
    cleanupConnection();
    setConnectionState(ConnectionState::Disconnected);
}

void MeshCoreDevice::onConnectionError(const QString &error)
{
    setErrorString(error);
    setConnectionState(ConnectionState::Error);
    Q_EMIT connectionError(error);
}

// Response handlers
void MeshCoreDevice::onSelfInfoReceived(const SelfInfo &selfInfo)
{
    m_selfInfo = selfInfo;
    Q_EMIT selfInfoChanged();
}

void MeshCoreDevice::onDeviceInfoReceived(const DeviceInfo &deviceInfo)
{
    m_deviceInfo = deviceInfo;
    Q_EMIT deviceInfoChanged();
}

void MeshCoreDevice::onContactsStarted(quint32 count)
{
    Q_UNUSED(count)
    if (!m_contactsSyncing) {
        m_contactModel.clear();
        Q_EMIT contactsCleared();
    }
    m_contactsSyncing = true;
}

void MeshCoreDevice::onContactReceived(const Contact &contact)
{
    m_contactModel.addContact(contact);
    Q_EMIT contactReceived(contact);
}

void MeshCoreDevice::onContactsEnded(quint32 mostRecentLastMod)
{
    Q_UNUSED(mostRecentLastMod)
    m_contactsSyncing = false;
}

void MeshCoreDevice::onChannelInfoReceived(const ChannelInfo &channelInfo)
{
    // Only add non-empty channels to the model
    if (!channelInfo.isEmpty()) {
        m_channelModel.updateChannel(channelInfo);
        Q_EMIT channelInfoReceived(channelInfo);
    }

    // If we're querying all channels, continue up to max 8 channels
    if (m_queryingChannels) {
        m_channelQueryIndex++;
        // MeshCore typically supports up to 8 channels (0-7)
        if (m_channelQueryIndex < 8 && m_connection) {
            m_connection->sendCommandGetChannel(m_channelQueryIndex);
        } else {
            m_queryingChannels = false;
            qDebug() << "Channel query complete, found" << m_channelModel.count() << "channels";
        }
    }
}

void MeshCoreDevice::onBatteryVoltageReceived(quint16 milliVolts)
{
    m_batteryMilliVolts = milliVolts;
    Q_EMIT batteryMilliVoltsChanged();
}

void MeshCoreDevice::onSentResponse(qint8 result, quint32 expectedAckCrc, quint32 estTimeout)
{
    Q_UNUSED(result)
    Q_EMIT messageSent(expectedAckCrc, estTimeout);
}

void MeshCoreDevice::onContactMsgReceived(const ContactMessage &message)
{
    m_messageModel.addContactMessage(message);
    Q_EMIT contactMessageReceived(message);

    // If syncing messages, continue
    if (m_syncingMessages && m_connection) {
        m_connection->sendCommandSyncNextMessage();
    }
}

void MeshCoreDevice::onChannelMsgReceived(const ChannelMessage &message)
{
    m_messageModel.addChannelMessage(message);
    Q_EMIT channelMessageReceived(message);

    // If syncing messages, continue
    if (m_syncingMessages && m_connection) {
        m_connection->sendCommandSyncNextMessage();
    }
}

void MeshCoreDevice::onNoMoreMessages()
{
    m_syncingMessages = false;
    Q_EMIT noMoreMessages();
}

void MeshCoreDevice::onExportContactReceived(const QByteArray &advertPacketBytes)
{
    Q_EMIT exportedContact(advertPacketBytes);
}

// Push notification handlers
void MeshCoreDevice::onNewAdvertPush(const Contact &contact)
{
    m_contactModel.updateContact(contact);
    Q_EMIT newAdvertReceived(contact);
}

void MeshCoreDevice::onPathUpdatedPush(const QByteArray &publicKey)
{
    Q_EMIT pathUpdated(publicKey);
}

void MeshCoreDevice::onSendConfirmedPush(quint32 ackCode, quint32 roundTrip)
{
    Q_EMIT sendConfirmed(ackCode, roundTrip);
}

void MeshCoreDevice::onMsgWaitingPush()
{
    Q_EMIT msgWaiting();
}

void MeshCoreDevice::onStatusResponsePush(const QByteArray &pubKeyPrefix, const RepeaterStats &stats)
{
    Q_EMIT repeaterStatusReceived(pubKeyPrefix, stats);
}

void MeshCoreDevice::onTelemetryResponsePush(const TelemetryData &telemetry)
{
    Q_EMIT telemetryReceived(telemetry);
}

void MeshCoreDevice::onTraceDataPush(const TraceData &traceData)
{
    Q_EMIT traceDataReceived(traceData);
}

// Device commands
void MeshCoreDevice::requestSelfInfo()
{
    if (m_connection) {
        m_connection->sendCommandAppStart();
    }
}

void MeshCoreDevice::requestContacts()
{
    qDebug() << "requestContacts called, connection:" << (m_connection ? "valid" : "null") 
             << "connected:" << (m_connection ? m_connection->isConnected() : false);
    if (m_connection) {
        m_contactsSyncing = true;
        m_contactModel.clear();
        m_connection->sendCommandGetContacts();
    }
}

void MeshCoreDevice::requestDeviceTime()
{
    if (m_connection) {
        m_connection->sendCommandGetDeviceTime();
    }
}

void MeshCoreDevice::syncDeviceTime()
{
    if (m_connection) {
        quint32 epochSecs = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());
        m_connection->sendCommandSetDeviceTime(epochSecs);
    }
}

void MeshCoreDevice::requestBatteryVoltage()
{
    if (m_connection) {
        m_connection->sendCommandGetBatteryVoltage();
    }
}

void MeshCoreDevice::requestChannel(int channelIndex)
{
    if (m_connection) {
        m_connection->sendCommandGetChannel(static_cast<quint8>(channelIndex));
    }
}

void MeshCoreDevice::requestAllChannels()
{
    if (m_connection) {
        m_queryingChannels = true;
        m_channelQueryIndex = 0;
        m_channelModel.clear();
        Q_EMIT channelsCleared();
        m_connection->sendCommandGetChannel(0);
    }
}

// Messaging
void MeshCoreDevice::sendTextMessage(const QByteArray &contactPublicKey, const QString &text)
{
    if (m_connection) {
        quint32 timestamp = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());
        m_connection->sendCommandSendTxtMsg(TxtType::Plain, 0, timestamp, contactPublicKey, text);
    }
}

void MeshCoreDevice::sendContactMessage(const QByteArray &contactPublicKey, const QString &text)
{
    sendTextMessage(contactPublicKey, text);
}

void MeshCoreDevice::sendTextMessageToName(const QString &contactName, const QString &text)
{
    Contact contact = m_contactModel.findByName(contactName);
    if (contact.publicKey().isEmpty()) {
        setErrorString(QStringLiteral("Contact not found: %1").arg(contactName));
        return;
    }
    sendTextMessage(contact.publicKey(), text);
}

void MeshCoreDevice::sendChannelMessage(int channelIndex, const QString &text)
{
    if (m_connection) {
        quint32 timestamp = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());
        m_connection->sendCommandSendChannelTxtMsg(TxtType::Plain, static_cast<quint8>(channelIndex), timestamp, text);
    }
}

void MeshCoreDevice::syncNextMessage()
{
    if (m_connection) {
        m_connection->sendCommandSyncNextMessage();
    }
}

void MeshCoreDevice::syncAllMessages()
{
    if (m_connection) {
        m_syncingMessages = true;
        m_connection->sendCommandSyncNextMessage();
    }
}

// Advert
void MeshCoreDevice::sendFloodAdvert()
{
    if (m_connection) {
        m_connection->sendCommandSendSelfAdvert(SelfAdvertType::Flood);
    }
}

void MeshCoreDevice::sendZeroHopAdvert()
{
    if (m_connection) {
        m_connection->sendCommandSendSelfAdvert(SelfAdvertType::ZeroHop);
    }
}

void MeshCoreDevice::setAdvertName(const QString &name)
{
    if (m_connection) {
        m_connection->sendCommandSetAdvertName(name);
    }
}

void MeshCoreDevice::setAdvertLocation(double latitude, double longitude)
{
    if (m_connection) {
        qint32 lat = static_cast<qint32>(latitude * 1e7);
        qint32 lon = static_cast<qint32>(longitude * 1e7);
        m_connection->sendCommandSetAdvertLatLon(lat, lon);
    }
}

// Radio settings
void MeshCoreDevice::setTxPower(int power)
{
    if (m_connection) {
        m_connection->sendCommandSetTxPower(static_cast<quint8>(power));
    }
}

void MeshCoreDevice::setRadioParams(quint32 freqHz, quint32 bwHz, int sf, int cr)
{
    if (m_connection) {
        m_connection->sendCommandSetRadioParams(freqHz, bwHz, static_cast<quint8>(sf), static_cast<quint8>(cr));
    }
}

// Contact management
void MeshCoreDevice::removeContact(const QByteArray &publicKey)
{
    if (m_connection) {
        m_connection->sendCommandRemoveContact(publicKey);
        m_contactModel.removeContact(publicKey);
    }
}

void MeshCoreDevice::resetContactPath(const QByteArray &publicKey)
{
    if (m_connection) {
        m_connection->sendCommandResetPath(publicKey);
    }
}

void MeshCoreDevice::shareContact(const QByteArray &publicKey)
{
    if (m_connection) {
        m_connection->sendCommandShareContact(publicKey);
    }
}

void MeshCoreDevice::exportContact(const QByteArray &publicKey)
{
    if (m_connection) {
        m_connection->sendCommandExportContact(publicKey);
    }
}

void MeshCoreDevice::importContact(const QByteArray &advertPacketBytes)
{
    if (m_connection) {
        m_connection->sendCommandImportContact(advertPacketBytes);
    }
}

// Channel management
void MeshCoreDevice::setChannel(int channelIndex, const QString &name, const QByteArray &secret)
{
    if (m_connection) {
        m_connection->sendCommandSetChannel(static_cast<quint8>(channelIndex), name, secret);
    }
}

void MeshCoreDevice::deleteChannel(int channelIndex)
{
    if (m_connection) {
        m_connection->sendCommandSetChannel(static_cast<quint8>(channelIndex), QString(), QByteArray(16, '\0'));
    }
}

// Advanced
void MeshCoreDevice::requestRepeaterStatus(const QByteArray &publicKey)
{
    if (m_connection) {
        m_connection->sendCommandSendStatusReq(publicKey);
    }
}

void MeshCoreDevice::requestTelemetry(const QByteArray &publicKey)
{
    if (m_connection) {
        m_connection->sendCommandSendTelemetryReq(publicKey);
    }
}

void MeshCoreDevice::sendTracePath(const QByteArray &path)
{
    if (m_connection) {
        quint32 tag = static_cast<quint32>(QRandomGenerator::global()->generate());
        m_connection->sendCommandSendTracePath(tag, 0, path);
    }
}

void MeshCoreDevice::reboot()
{
    if (m_connection) {
        m_connection->sendCommandReboot();
    }
}

void MeshCoreDevice::setManualAddContacts(bool manual)
{
    if (m_connection) {
        m_connection->sendCommandSetOtherParams(manual);
    }
}

// RX Log push handler
void MeshCoreDevice::onLogRxDataPush(double snr, qint8 rssi, const QByteArray &rawData)
{
    // Add to the RX log model (which will check if logging is enabled)
    m_rxLogModel.addEntry(snr, rssi, rawData);
    Q_EMIT rxLogEntry(snr, rssi, rawData);
}

} // namespace MeshCore
