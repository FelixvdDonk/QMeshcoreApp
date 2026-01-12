#include "MeshCoreDeviceController.h"
#include <QDebug>

namespace MeshCore {

MeshCoreDeviceController::MeshCoreDeviceController(QObject *parent)
    : QObject(parent)
{
    // Create device on worker thread
    m_device = new MeshCoreDevice(nullptr);  // No parent - will be moved to thread
    m_device->moveToThread(&m_workerThread);

    // Clean up device when thread finishes
    connect(&m_workerThread, &QThread::finished, m_device, &QObject::deleteLater);

    // Set up all signal/slot connections
    setupConnections();

    // Start the worker thread
    m_workerThread.setObjectName(QStringLiteral("MeshCoreDeviceWorker"));
    m_workerThread.start();

    qDebug() << "MeshCoreDeviceController: Worker thread started";
}

MeshCoreDeviceController::~MeshCoreDeviceController()
{
    // Request thread to quit and wait
    m_workerThread.quit();
    m_workerThread.wait();
    qDebug() << "MeshCoreDeviceController: Worker thread stopped";
}

void MeshCoreDeviceController::setupConnections()
{
    // All connections are Qt::QueuedConnection by default when crossing threads

    // === Command signals from controller to device ===
    connect(this, &MeshCoreDeviceController::doStartBleScan,
            m_device, &MeshCoreDevice::startBleScan);
    connect(this, &MeshCoreDeviceController::doStopBleScan,
            m_device, &MeshCoreDevice::stopBleScan);
    connect(this, &MeshCoreDeviceController::doConnectBle,
            m_device, &MeshCoreDevice::connectBle);
    connect(this, &MeshCoreDeviceController::doConnectBleByAddress,
            m_device, &MeshCoreDevice::connectBleByAddress);
    connect(this, &MeshCoreDeviceController::doRefreshSerialPorts,
            m_device, &MeshCoreDevice::refreshSerialPorts);
    connect(this, &MeshCoreDeviceController::doConnectSerial,
            m_device, &MeshCoreDevice::connectSerial);
    connect(this, &MeshCoreDeviceController::doConnectSerialByIndex,
            m_device, &MeshCoreDevice::connectSerialByIndex);
    connect(this, &MeshCoreDeviceController::doDisconnect,
            m_device, &MeshCoreDevice::disconnect);
    connect(this, &MeshCoreDeviceController::doRequestSelfInfo,
            m_device, &MeshCoreDevice::requestSelfInfo);
    connect(this, &MeshCoreDeviceController::doRequestContacts,
            m_device, &MeshCoreDevice::requestContacts);
    connect(this, &MeshCoreDeviceController::doRequestDeviceTime,
            m_device, &MeshCoreDevice::requestDeviceTime);
    connect(this, &MeshCoreDeviceController::doSyncDeviceTime,
            m_device, &MeshCoreDevice::syncDeviceTime);
    connect(this, &MeshCoreDeviceController::doRequestBatteryVoltage,
            m_device, &MeshCoreDevice::requestBatteryVoltage);
    connect(this, &MeshCoreDeviceController::doRequestChannel,
            m_device, &MeshCoreDevice::requestChannel);
    connect(this, &MeshCoreDeviceController::doRequestAllChannels,
            m_device, &MeshCoreDevice::requestAllChannels);
    connect(this, &MeshCoreDeviceController::doSendTextMessage,
            m_device, &MeshCoreDevice::sendTextMessage);
    connect(this, &MeshCoreDeviceController::doSendTextMessageToName,
            m_device, &MeshCoreDevice::sendTextMessageToName);
    connect(this, &MeshCoreDeviceController::doSendChannelMessage,
            m_device, &MeshCoreDevice::sendChannelMessage);
    connect(this, &MeshCoreDeviceController::doSyncNextMessage,
            m_device, &MeshCoreDevice::syncNextMessage);
    connect(this, &MeshCoreDeviceController::doSyncAllMessages,
            m_device, &MeshCoreDevice::syncAllMessages);
    connect(this, &MeshCoreDeviceController::doSendFloodAdvert,
            m_device, &MeshCoreDevice::sendFloodAdvert);
    connect(this, &MeshCoreDeviceController::doSendZeroHopAdvert,
            m_device, &MeshCoreDevice::sendZeroHopAdvert);
    connect(this, &MeshCoreDeviceController::doSetAdvertName,
            m_device, &MeshCoreDevice::setAdvertName);
    connect(this, &MeshCoreDeviceController::doSetAdvertLocation,
            m_device, &MeshCoreDevice::setAdvertLocation);
    connect(this, &MeshCoreDeviceController::doSetTxPower,
            m_device, &MeshCoreDevice::setTxPower);
    connect(this, &MeshCoreDeviceController::doSetRadioParams,
            m_device, &MeshCoreDevice::setRadioParams);
    connect(this, &MeshCoreDeviceController::doRemoveContact,
            m_device, &MeshCoreDevice::removeContact);
    connect(this, &MeshCoreDeviceController::doResetContactPath,
            m_device, &MeshCoreDevice::resetContactPath);
    connect(this, &MeshCoreDeviceController::doShareContact,
            m_device, &MeshCoreDevice::shareContact);
    connect(this, &MeshCoreDeviceController::doExportContact,
            m_device, &MeshCoreDevice::exportContact);
    connect(this, &MeshCoreDeviceController::doImportContact,
            m_device, &MeshCoreDevice::importContact);
    connect(this, &MeshCoreDeviceController::doSetChannel,
            m_device, &MeshCoreDevice::setChannel);
    connect(this, &MeshCoreDeviceController::doDeleteChannel,
            m_device, &MeshCoreDevice::deleteChannel);
    connect(this, &MeshCoreDeviceController::doRequestRepeaterStatus,
            m_device, &MeshCoreDevice::requestRepeaterStatus);
    connect(this, &MeshCoreDeviceController::doRequestTelemetry,
            m_device, &MeshCoreDevice::requestTelemetry);
    connect(this, &MeshCoreDeviceController::doSendTracePath,
            m_device, &MeshCoreDevice::sendTracePath);
    connect(this, &MeshCoreDeviceController::doReboot,
            m_device, &MeshCoreDevice::reboot);
    connect(this, &MeshCoreDeviceController::doSetManualAddContacts,
            m_device, &MeshCoreDevice::setManualAddContacts);

    // === State change signals from device to controller ===
    connect(m_device, &MeshCoreDevice::connectionStateChanged,
            this, &MeshCoreDeviceController::onConnectionStateChanged);
    connect(m_device, &MeshCoreDevice::connectionTypeChanged,
            this, &MeshCoreDeviceController::onConnectionTypeChanged);
    connect(m_device, &MeshCoreDevice::errorStringChanged,
            this, &MeshCoreDeviceController::onErrorStringChanged);
    connect(m_device, &MeshCoreDevice::selfInfoChanged,
            this, &MeshCoreDeviceController::onSelfInfoChanged);
    connect(m_device, &MeshCoreDevice::deviceInfoChanged,
            this, &MeshCoreDeviceController::onDeviceInfoChanged);
    connect(m_device, &MeshCoreDevice::batteryMilliVoltsChanged,
            this, &MeshCoreDeviceController::onBatteryMilliVoltsChanged);
    connect(m_device, &MeshCoreDevice::scanningChanged,
            this, &MeshCoreDeviceController::onScanningChanged);
    connect(m_device, &MeshCoreDevice::discoveredBleDevicesChanged,
            this, &MeshCoreDeviceController::onDiscoveredBleDevicesChanged);
    connect(m_device, &MeshCoreDevice::availableSerialPortsChanged,
            this, &MeshCoreDeviceController::onAvailableSerialPortsChanged);

    // === Event signals - forward directly ===
    connect(m_device, &MeshCoreDevice::connectionError,
            this, &MeshCoreDeviceController::connectionError);
    connect(m_device, &MeshCoreDevice::messageSent,
            this, &MeshCoreDeviceController::messageSent);
    connect(m_device, &MeshCoreDevice::noMoreMessages,
            this, &MeshCoreDeviceController::noMoreMessages);
    connect(m_device, &MeshCoreDevice::newAdvertReceived,
            this, &MeshCoreDeviceController::newAdvertReceived);
    connect(m_device, &MeshCoreDevice::pathUpdated,
            this, &MeshCoreDeviceController::pathUpdated);
    connect(m_device, &MeshCoreDevice::sendConfirmed,
            this, &MeshCoreDeviceController::sendConfirmed);
    connect(m_device, &MeshCoreDevice::msgWaiting,
            this, &MeshCoreDeviceController::msgWaiting);
    connect(m_device, &MeshCoreDevice::repeaterStatusReceived,
            this, &MeshCoreDeviceController::repeaterStatusReceived);
    connect(m_device, &MeshCoreDevice::telemetryReceived,
            this, &MeshCoreDeviceController::telemetryReceived);
    connect(m_device, &MeshCoreDevice::traceDataReceived,
            this, &MeshCoreDeviceController::traceDataReceived);
    connect(m_device, &MeshCoreDevice::exportedContact,
            this, &MeshCoreDeviceController::exportedContact);

    // === Model updates - sync to local models ===
    // Contact model updates (from contact sync and new adverts)
    connect(m_device, &MeshCoreDevice::contactReceived,
            this, &MeshCoreDeviceController::onContactReceived);
    connect(m_device, &MeshCoreDevice::newAdvertReceived,
            this, &MeshCoreDeviceController::onContactReceived);
    connect(m_device, &MeshCoreDevice::contactsCleared,
            this, &MeshCoreDeviceController::onContactsCleared);
    
    // Channel model updates
    connect(m_device, &MeshCoreDevice::channelInfoReceived,
            this, &MeshCoreDeviceController::onChannelReceived);
    connect(m_device, &MeshCoreDevice::channelsCleared,
            this, &MeshCoreDeviceController::onChannelsCleared);
    
    // Message updates
    connect(m_device, &MeshCoreDevice::contactMessageReceived,
            this, &MeshCoreDeviceController::onContactMessageReceived);
    connect(m_device, &MeshCoreDevice::channelMessageReceived,
            this, &MeshCoreDeviceController::onChannelMessageReceived);
    
    // RX Log updates
    connect(m_device, &MeshCoreDevice::rxLogEntry,
            this, &MeshCoreDeviceController::onRxLogEntry);
}

// === Public slots - forward to worker via signals ===

void MeshCoreDeviceController::startBleScan()
{
    Q_EMIT doStartBleScan();
}

void MeshCoreDeviceController::stopBleScan()
{
    Q_EMIT doStopBleScan();
}

void MeshCoreDeviceController::connectBle(int deviceIndex)
{
    Q_EMIT doConnectBle(deviceIndex);
}

void MeshCoreDeviceController::connectBleByAddress(const QString &address)
{
    Q_EMIT doConnectBleByAddress(address);
}

void MeshCoreDeviceController::refreshSerialPorts()
{
    Q_EMIT doRefreshSerialPorts();
}

void MeshCoreDeviceController::connectSerial(const QString &portName, int baudRate)
{
    Q_EMIT doConnectSerial(portName, baudRate);
}

void MeshCoreDeviceController::connectSerialByIndex(int portIndex, int baudRate)
{
    Q_EMIT doConnectSerialByIndex(portIndex, baudRate);
}

void MeshCoreDeviceController::disconnect()
{
    Q_EMIT doDisconnect();
}

void MeshCoreDeviceController::requestSelfInfo()
{
    Q_EMIT doRequestSelfInfo();
}

void MeshCoreDeviceController::requestContacts()
{
    m_contactModel.clear();  // Clear local model
    Q_EMIT doRequestContacts();
}

void MeshCoreDeviceController::requestDeviceTime()
{
    Q_EMIT doRequestDeviceTime();
}

void MeshCoreDeviceController::syncDeviceTime()
{
    Q_EMIT doSyncDeviceTime();
}

void MeshCoreDeviceController::requestBatteryVoltage()
{
    Q_EMIT doRequestBatteryVoltage();
}

void MeshCoreDeviceController::requestChannel(int channelIndex)
{
    Q_EMIT doRequestChannel(channelIndex);
}

void MeshCoreDeviceController::requestAllChannels()
{
    m_channelModel.clear();  // Clear local model
    Q_EMIT doRequestAllChannels();
}

void MeshCoreDeviceController::sendTextMessage(const QByteArray &contactPublicKey, const QString &text)
{
    Q_EMIT doSendTextMessage(contactPublicKey, text);
}

void MeshCoreDeviceController::sendTextMessageToName(const QString &contactName, const QString &text)
{
    Q_EMIT doSendTextMessageToName(contactName, text);
}

void MeshCoreDeviceController::sendContactMessage(const QByteArray &contactPublicKey, const QString &text)
{
    Q_EMIT doSendTextMessage(contactPublicKey, text);
}

void MeshCoreDeviceController::sendChannelMessage(int channelIndex, const QString &text)
{
    Q_EMIT doSendChannelMessage(channelIndex, text);
}

void MeshCoreDeviceController::syncNextMessage()
{
    Q_EMIT doSyncNextMessage();
}

void MeshCoreDeviceController::syncAllMessages()
{
    Q_EMIT doSyncAllMessages();
}

void MeshCoreDeviceController::sendFloodAdvert()
{
    Q_EMIT doSendFloodAdvert();
}

void MeshCoreDeviceController::sendZeroHopAdvert()
{
    Q_EMIT doSendZeroHopAdvert();
}

void MeshCoreDeviceController::setAdvertName(const QString &name)
{
    Q_EMIT doSetAdvertName(name);
}

void MeshCoreDeviceController::setAdvertLocation(double latitude, double longitude)
{
    Q_EMIT doSetAdvertLocation(latitude, longitude);
}

void MeshCoreDeviceController::setTxPower(int power)
{
    Q_EMIT doSetTxPower(power);
}

void MeshCoreDeviceController::setRadioParams(quint32 freqHz, quint32 bwHz, int sf, int cr)
{
    Q_EMIT doSetRadioParams(freqHz, bwHz, sf, cr);
}

void MeshCoreDeviceController::removeContact(const QByteArray &publicKey)
{
    m_contactModel.removeContact(publicKey);  // Update local model
    Q_EMIT doRemoveContact(publicKey);
}

void MeshCoreDeviceController::resetContactPath(const QByteArray &publicKey)
{
    Q_EMIT doResetContactPath(publicKey);
}

void MeshCoreDeviceController::shareContact(const QByteArray &publicKey)
{
    Q_EMIT doShareContact(publicKey);
}

void MeshCoreDeviceController::exportContact(const QByteArray &publicKey)
{
    Q_EMIT doExportContact(publicKey);
}

void MeshCoreDeviceController::importContact(const QByteArray &advertPacketBytes)
{
    Q_EMIT doImportContact(advertPacketBytes);
}

void MeshCoreDeviceController::setChannel(int channelIndex, const QString &name, const QByteArray &secret)
{
    Q_EMIT doSetChannel(channelIndex, name, secret);
}

void MeshCoreDeviceController::deleteChannel(int channelIndex)
{
    Q_EMIT doDeleteChannel(channelIndex);
}

void MeshCoreDeviceController::requestRepeaterStatus(const QByteArray &publicKey)
{
    Q_EMIT doRequestRepeaterStatus(publicKey);
}

void MeshCoreDeviceController::requestTelemetry(const QByteArray &publicKey)
{
    Q_EMIT doRequestTelemetry(publicKey);
}

void MeshCoreDeviceController::sendTracePath(const QByteArray &path)
{
    Q_EMIT doSendTracePath(path);
}

void MeshCoreDeviceController::reboot()
{
    Q_EMIT doReboot();
}

void MeshCoreDeviceController::setManualAddContacts(bool manual)
{
    Q_EMIT doSetManualAddContacts(manual);
}

QVariantList MeshCoreDeviceController::availableSerialPorts() const
{
    // This needs to be fetched from device - for now return cached or query synchronously
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

// === Private slots - update cached state from worker ===

void MeshCoreDeviceController::onConnectionStateChanged()
{
    m_connectionState = m_device->connectionState();
    Q_EMIT connectionStateChanged();
    Q_EMIT connectedChanged();
}

void MeshCoreDeviceController::onConnectionTypeChanged()
{
    m_connectionType = m_device->connectionType();
    Q_EMIT connectionTypeChanged();
}

void MeshCoreDeviceController::onErrorStringChanged()
{
    m_errorString = m_device->errorString();
    Q_EMIT errorStringChanged();
}

void MeshCoreDeviceController::onSelfInfoChanged()
{
    m_selfInfo = m_device->selfInfo();
    Q_EMIT selfInfoChanged();
}

void MeshCoreDeviceController::onDeviceInfoChanged()
{
    m_deviceInfo = m_device->deviceInfo();
    Q_EMIT deviceInfoChanged();
}

void MeshCoreDeviceController::onBatteryMilliVoltsChanged()
{
    m_batteryMilliVolts = m_device->batteryMilliVolts();
    Q_EMIT batteryMilliVoltsChanged();
}

void MeshCoreDeviceController::onScanningChanged()
{
    m_scanning = m_device->isScanning();
    Q_EMIT scanningChanged();
}

void MeshCoreDeviceController::onDiscoveredBleDevicesChanged()
{
    m_discoveredBleDevices = m_device->discoveredBleDevices();
    Q_EMIT discoveredBleDevicesChanged();
}

void MeshCoreDeviceController::onAvailableSerialPortsChanged()
{
    Q_EMIT availableSerialPortsChanged();
}

// === Model sync slots ===

void MeshCoreDeviceController::onContactReceived(const Contact &contact)
{
    m_contactModel.updateContact(contact);
}

void MeshCoreDeviceController::onContactsCleared()
{
    m_contactModel.clear();
}

void MeshCoreDeviceController::onChannelReceived(const ChannelInfo &channel)
{
    m_channelModel.updateChannel(channel);
}

void MeshCoreDeviceController::onChannelsCleared()
{
    m_channelModel.clear();
}

void MeshCoreDeviceController::onContactMessageReceived(const ContactMessage &message)
{
    m_messageModel.addContactMessage(message);
    Q_EMIT contactMessageReceived(message);
}

void MeshCoreDeviceController::onChannelMessageReceived(const ChannelMessage &message)
{
    m_messageModel.addChannelMessage(message);
    Q_EMIT channelMessageReceived(message);
}

void MeshCoreDeviceController::onRxLogEntry(double snr, qint8 rssi, const QByteArray &rawData)
{
    m_rxLogModel.addEntry(snr, rssi, rawData);
}

} // namespace MeshCore
