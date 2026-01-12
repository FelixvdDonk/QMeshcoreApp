#ifndef MESHCOREDEVICECONTROLLER_H
#define MESHCOREDEVICECONTROLLER_H

#include <QObject>
#include <QThread>
#include <QtQml/qqmlregistration.h>

#include "MeshCoreDevice.h"

namespace MeshCore {

/**
 * @brief QML-facing controller that manages MeshCoreDevice on a worker thread
 *
 * This class provides the same API as MeshCoreDevice but runs the actual
 * device operations on a separate thread to keep the UI responsive.
 * All communication happens via queued signal/slot connections.
 */
class MeshCoreDeviceController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    // Forward enums from MeshCoreDevice
    using ConnectionState = MeshCoreDevice::ConnectionState;
    using ConnectionType = MeshCoreDevice::ConnectionType;
    using AdvertTypeEnum = MeshCoreDevice::AdvertTypeEnum;

private:
    // Properties - mirror MeshCoreDevice
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(ConnectionType connectionType READ connectionType NOTIFY connectionTypeChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

    Q_PROPERTY(SelfInfo selfInfo READ selfInfo NOTIFY selfInfoChanged)
    Q_PROPERTY(DeviceInfo deviceInfo READ deviceInfo NOTIFY deviceInfoChanged)
    Q_PROPERTY(quint16 batteryMilliVolts READ batteryMilliVolts NOTIFY batteryMilliVoltsChanged)
    Q_PROPERTY(double batteryVolts READ batteryVolts NOTIFY batteryMilliVoltsChanged)

    // Models live on the main thread for QML binding
    Q_PROPERTY(ContactModel* contacts READ contacts CONSTANT)
    Q_PROPERTY(ChannelModel* channels READ channels CONSTANT)
    Q_PROPERTY(MessageModel* messages READ messages CONSTANT)
    Q_PROPERTY(RxLogModel* rxLog READ rxLog CONSTANT)

    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QVariantList discoveredBleDevices READ discoveredBleDevices NOTIFY discoveredBleDevicesChanged)
    Q_PROPERTY(QVariantList availableSerialPorts READ availableSerialPorts NOTIFY availableSerialPortsChanged)

public:
    explicit MeshCoreDeviceController(QObject *parent = nullptr);
    ~MeshCoreDeviceController() override;

    // Property getters (cached values from worker)
    [[nodiscard]] ConnectionState connectionState() const { return m_connectionState; }
    [[nodiscard]] ConnectionType connectionType() const { return m_connectionType; }
    [[nodiscard]] bool isConnected() const { return m_connectionState == ConnectionState::Connected; }
    [[nodiscard]] QString errorString() const { return m_errorString; }
    [[nodiscard]] SelfInfo selfInfo() const { return m_selfInfo; }
    [[nodiscard]] DeviceInfo deviceInfo() const { return m_deviceInfo; }
    [[nodiscard]] quint16 batteryMilliVolts() const { return m_batteryMilliVolts; }
    [[nodiscard]] double batteryVolts() const { return m_batteryMilliVolts / 1000.0; }

    // Models (on main thread)
    [[nodiscard]] ContactModel *contacts() { return &m_contactModel; }
    [[nodiscard]] ChannelModel *channels() { return &m_channelModel; }
    [[nodiscard]] MessageModel *messages() { return &m_messageModel; }
    [[nodiscard]] RxLogModel *rxLog() { return &m_rxLogModel; }

    [[nodiscard]] bool isScanning() const { return m_scanning; }
    [[nodiscard]] QVariantList discoveredBleDevices() const { return m_discoveredBleDevices; }
    [[nodiscard]] QVariantList availableSerialPorts() const;

public Q_SLOTS:
    // BLE operations - forwarded to worker
    void startBleScan();
    void stopBleScan();
    void connectBle(int deviceIndex);
    void connectBleByAddress(const QString &address);

    // Serial operations
    void refreshSerialPorts();
    void connectSerial(const QString &portName, int baudRate = 115200);
    void connectSerialByIndex(int portIndex, int baudRate = 115200);

    // General connection
    void disconnect();

    // Device commands
    void requestSelfInfo();
    void requestContacts();
    void requestDeviceTime();
    void syncDeviceTime();
    void requestBatteryVoltage();
    void requestChannel(int channelIndex);
    void requestAllChannels();

    // Messaging
    void sendTextMessage(const QByteArray &contactPublicKey, const QString &text);
    void sendTextMessageToName(const QString &contactName, const QString &text);
    void sendContactMessage(const QByteArray &contactPublicKey, const QString &text);
    void sendChannelMessage(int channelIndex, const QString &text);
    void syncNextMessage();
    void syncAllMessages();

    // Advert
    void sendFloodAdvert();
    void sendZeroHopAdvert();
    void setAdvertName(const QString &name);
    void setAdvertLocation(double latitude, double longitude);

    // Radio settings
    void setTxPower(int power);
    void setRadioParams(quint32 freqHz, quint32 bwHz, int sf, int cr);

    // Contact management
    void removeContact(const QByteArray &publicKey);
    void resetContactPath(const QByteArray &publicKey);
    void shareContact(const QByteArray &publicKey);
    void exportContact(const QByteArray &publicKey = QByteArray());
    void importContact(const QByteArray &advertPacketBytes);

    // Channel management
    void setChannel(int channelIndex, const QString &name, const QByteArray &secret);
    void deleteChannel(int channelIndex);

    // Advanced
    void requestRepeaterStatus(const QByteArray &publicKey);
    void requestTelemetry(const QByteArray &publicKey);
    void sendTracePath(const QByteArray &path);
    void reboot();
    void setManualAddContacts(bool manual);

Q_SIGNALS:
    // Property change signals
    void connectionStateChanged();
    void connectionTypeChanged();
    void connectedChanged();
    void errorStringChanged();
    void selfInfoChanged();
    void deviceInfoChanged();
    void batteryMilliVoltsChanged();
    void scanningChanged();
    void discoveredBleDevicesChanged();
    void availableSerialPortsChanged();

    // Event signals
    void connectionError(const QString &error);
    void messageSent(quint32 expectedAckCrc, quint32 estTimeout);
    void contactMessageReceived(const ContactMessage &message);
    void channelMessageReceived(const ChannelMessage &message);
    void noMoreMessages();
    void newAdvertReceived(const Contact &contact);
    void pathUpdated(const QByteArray &publicKey);
    void sendConfirmed(quint32 ackCode, quint32 roundTrip);
    void msgWaiting();
    void repeaterStatusReceived(const QByteArray &publicKey, const RepeaterStats &stats);
    void telemetryReceived(const TelemetryData &telemetry);
    void traceDataReceived(const TraceData &traceData);
    void exportedContact(const QByteArray &advertPacketBytes);

    // Internal signals to worker thread
    void doStartBleScan();
    void doStopBleScan();
    void doConnectBle(int deviceIndex);
    void doConnectBleByAddress(const QString &address);
    void doRefreshSerialPorts();
    void doConnectSerial(const QString &portName, int baudRate);
    void doConnectSerialByIndex(int portIndex, int baudRate);
    void doDisconnect();
    void doRequestSelfInfo();
    void doRequestContacts();
    void doRequestDeviceTime();
    void doSyncDeviceTime();
    void doRequestBatteryVoltage();
    void doRequestChannel(int channelIndex);
    void doRequestAllChannels();
    void doSendTextMessage(const QByteArray &contactPublicKey, const QString &text);
    void doSendTextMessageToName(const QString &contactName, const QString &text);
    void doSendChannelMessage(int channelIndex, const QString &text);
    void doSyncNextMessage();
    void doSyncAllMessages();
    void doSendFloodAdvert();
    void doSendZeroHopAdvert();
    void doSetAdvertName(const QString &name);
    void doSetAdvertLocation(double latitude, double longitude);
    void doSetTxPower(int power);
    void doSetRadioParams(quint32 freqHz, quint32 bwHz, int sf, int cr);
    void doRemoveContact(const QByteArray &publicKey);
    void doResetContactPath(const QByteArray &publicKey);
    void doShareContact(const QByteArray &publicKey);
    void doExportContact(const QByteArray &publicKey);
    void doImportContact(const QByteArray &advertPacketBytes);
    void doSetChannel(int channelIndex, const QString &name, const QByteArray &secret);
    void doDeleteChannel(int channelIndex);
    void doRequestRepeaterStatus(const QByteArray &publicKey);
    void doRequestTelemetry(const QByteArray &publicKey);
    void doSendTracePath(const QByteArray &path);
    void doReboot();
    void doSetManualAddContacts(bool manual);

private Q_SLOTS:
    // Slots to receive updates from worker
    void onConnectionStateChanged();
    void onConnectionTypeChanged();
    void onErrorStringChanged();
    void onSelfInfoChanged();
    void onDeviceInfoChanged();
    void onBatteryMilliVoltsChanged();
    void onScanningChanged();
    void onDiscoveredBleDevicesChanged();
    void onAvailableSerialPortsChanged();

    // Forward model updates from worker
    void onContactReceived(const Contact &contact);
    void onContactsCleared();
    void onChannelReceived(const ChannelInfo &channel);
    void onChannelsCleared();
    void onContactMessageReceived(const ContactMessage &message);
    void onChannelMessageReceived(const ChannelMessage &message);
    void onRxLogEntry(double snr, qint8 rssi, const QByteArray &rawData);

private:
    void setupConnections();

    QThread m_workerThread;
    MeshCoreDevice *m_device = nullptr;  // Lives on worker thread

    // Cached state (main thread copies)
    ConnectionState m_connectionState = ConnectionState::Disconnected;
    ConnectionType m_connectionType = ConnectionType::None;
    QString m_errorString;
    SelfInfo m_selfInfo;
    DeviceInfo m_deviceInfo;
    quint16 m_batteryMilliVolts = 0;
    bool m_scanning = false;
    QVariantList m_discoveredBleDevices;

    // Models on main thread
    ContactModel m_contactModel;
    ChannelModel m_channelModel;
    MessageModel m_messageModel;
    RxLogModel m_rxLogModel;
};

} // namespace MeshCore

#endif // MESHCOREDEVICECONTROLLER_H
