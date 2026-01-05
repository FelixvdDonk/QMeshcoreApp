#ifndef MESHCOREDEVICE_H
#define MESHCOREDEVICE_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QSerialPortInfo>
#include <QtQml/qqmlregistration.h>
#include <memory>

#include "MeshCoreConstants.h"
#include "types/SelfInfo.h"
#include "types/DeviceInfo.h"
#include "types/Contact.h"
#include "types/ChannelInfo.h"
#include "types/ContactMessage.h"
#include "types/ChannelMessage.h"
#include "types/RepeaterStats.h"
#include "types/TraceData.h"
#include "types/TelemetryData.h"
#include "models/ContactModel.h"
#include "models/ChannelModel.h"
#include "models/MessageModel.h"
#include "models/RxLogModel.h"

namespace MeshCore {

class MeshCoreConnection;
class BleConnection;
class SerialConnection;

/**
 * @brief Main interface class for communicating with a MeshCore device
 *
 * This class provides a high-level, QML-friendly API for connecting to
 * and communicating with MeshCore devices over BLE or Serial (USB).
 *
 * Example QML usage:
 * @code
 * MeshCoreDevice {
 *     id: device
 *     onConnectedChanged: console.log("Connected:", connected)
 *     onSelfInfoChanged: console.log("Device name:", selfInfo.name)
 * }
 *
 * Button {
 *     text: "Scan BLE"
 *     onClicked: device.startBleScan()
 * }
 * @endcode
 */
class MeshCoreDevice : public QObject
{
    Q_OBJECT

public:
    // Enums exposed to QML
    enum ConnectionState {
        Disconnected = 0,
        Connecting,
        Connected,
        Error
    };
    Q_ENUM(ConnectionState)

    enum ConnectionType {
        None = 0,
        Ble,
        Serial
    };
    Q_ENUM(ConnectionType)

    // Advert types - must match MeshCoreConstants::AdvertType values
    enum AdvertTypeEnum {
        AdvertTypeNone = 0,      // Unknown/unset
        AdvertTypeChat = 1,      // Chat client node
        AdvertTypeRepeater = 2,  // Repeater node
        AdvertTypeRoom = 3       // Room server
    };
    Q_ENUM(AdvertTypeEnum)

private:
    // Connection state
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(ConnectionType connectionType READ connectionType NOTIFY connectionTypeChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

    // Device info
    Q_PROPERTY(SelfInfo selfInfo READ selfInfo NOTIFY selfInfoChanged)
    Q_PROPERTY(DeviceInfo deviceInfo READ deviceInfo NOTIFY deviceInfoChanged)
    Q_PROPERTY(quint16 batteryMilliVolts READ batteryMilliVolts NOTIFY batteryMilliVoltsChanged)
    Q_PROPERTY(double batteryVolts READ batteryVolts NOTIFY batteryMilliVoltsChanged)

    // Models (for QML ListView binding)
    Q_PROPERTY(ContactModel* contacts READ contacts CONSTANT)
    Q_PROPERTY(ChannelModel* channels READ channels CONSTANT)
    Q_PROPERTY(MessageModel* messages READ messages CONSTANT)
    Q_PROPERTY(RxLogModel* rxLog READ rxLog CONSTANT)

    // BLE scanning
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QVariantList discoveredBleDevices READ discoveredBleDevices NOTIFY discoveredBleDevicesChanged)

    // Serial ports
    Q_PROPERTY(QVariantList availableSerialPorts READ availableSerialPorts NOTIFY availableSerialPortsChanged)

public:
    explicit MeshCoreDevice(QObject *parent = nullptr);
    ~MeshCoreDevice() override;

    // Property getters
    [[nodiscard]] ConnectionState connectionState() const { return m_connectionState; }
    [[nodiscard]] ConnectionType connectionType() const { return m_connectionType; }
    [[nodiscard]] bool isConnected() const { return m_connectionState == ConnectionState::Connected; }
    [[nodiscard]] QString errorString() const { return m_errorString; }
    [[nodiscard]] SelfInfo selfInfo() const { return m_selfInfo; }
    [[nodiscard]] DeviceInfo deviceInfo() const { return m_deviceInfo; }
    [[nodiscard]] quint16 batteryMilliVolts() const { return m_batteryMilliVolts; }
    [[nodiscard]] double batteryVolts() const { return m_batteryMilliVolts / 1000.0; }

    // Model getters
    [[nodiscard]] ContactModel *contacts() { return &m_contactModel; }
    [[nodiscard]] ChannelModel *channels() { return &m_channelModel; }
    [[nodiscard]] MessageModel *messages() { return &m_messageModel; }
    [[nodiscard]] RxLogModel *rxLog() { return &m_rxLogModel; }

    // BLE scanning
    [[nodiscard]] bool isScanning() const { return m_scanning; }
    [[nodiscard]] QVariantList discoveredBleDevices() const { return m_discoveredBleDevices; }

    // Serial ports
    [[nodiscard]] QVariantList availableSerialPorts() const;

public Q_SLOTS:
    // BLE operations
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

    // Device commands (high-level)
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
    void sendContactMessage(const QByteArray &contactPublicKey, const QString &text);  // Alias for sendTextMessage
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

    // Low-level access
    void setManualAddContacts(bool manual);

Q_SIGNALS:
    // Property signals
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
    void contactReceived(const Contact &contact);
    void contactMessageReceived(const ContactMessage &message);
    void channelMessageReceived(const ChannelMessage &message);
    void messageSent(quint32 expectedAckCrc, quint32 estTimeoutMs);
    void sendConfirmed(quint32 ackCode, quint32 roundTripMs);
    void newAdvertReceived(const Contact &contact);
    void pathUpdated(const QByteArray &publicKey);
    void repeaterStatusReceived(const RepeaterStats &stats);
    void telemetryReceived(const TelemetryData &telemetry);
    void traceDataReceived(const TraceData &traceData);
    void exportedContact(const QByteArray &advertPacketBytes);
    void msgWaiting();
    void noMoreMessages();

private Q_SLOTS:
    // BLE discovery
    void onBleDeviceDiscovered(const QBluetoothDeviceInfo &deviceInfo);
    void onBleScanFinished();
    void onBleScanError(QBluetoothDeviceDiscoveryAgent::Error error);

    // Connection events
    void onConnectionConnected();
    void onConnectionDisconnected();
    void onConnectionError(const QString &error);

    // Response handlers
    void onSelfInfoReceived(const SelfInfo &selfInfo);
    void onDeviceInfoReceived(const DeviceInfo &deviceInfo);
    void onContactsStarted(quint32 count);
    void onContactReceived(const Contact &contact);
    void onContactsEnded(quint32 mostRecentLastMod);
    void onChannelInfoReceived(const ChannelInfo &channelInfo);
    void onBatteryVoltageReceived(quint16 milliVolts);
    void onSentResponse(qint8 result, quint32 expectedAckCrc, quint32 estTimeout);
    void onContactMsgReceived(const ContactMessage &message);
    void onChannelMsgReceived(const ChannelMessage &message);
    void onNoMoreMessages();
    void onExportContactReceived(const QByteArray &advertPacketBytes);

    // Push notifications
    void onNewAdvertPush(const Contact &contact);
    void onPathUpdatedPush(const QByteArray &publicKey);
    void onSendConfirmedPush(quint32 ackCode, quint32 roundTrip);
    void onMsgWaitingPush();
    void onStatusResponsePush(const QByteArray &pubKeyPrefix, const RepeaterStats &stats);
    void onTelemetryResponsePush(const TelemetryData &telemetry);
    void onTraceDataPush(const TraceData &traceData);
    void onLogRxDataPush(double snr, qint8 rssi, const QByteArray &rawData);

private:
    void setConnectionState(ConnectionState state);
    void setErrorString(const QString &error);
    void setupConnectionSignals();
    void cleanupConnection();

    // Connection
    std::unique_ptr<MeshCoreConnection> m_connection;
    ConnectionState m_connectionState = ConnectionState::Disconnected;
    ConnectionType m_connectionType = ConnectionType::None;
    QString m_errorString;

    // Device state
    SelfInfo m_selfInfo;
    DeviceInfo m_deviceInfo;
    quint16 m_batteryMilliVolts = 0;

    // Models
    ContactModel m_contactModel;
    ChannelModel m_channelModel;
    MessageModel m_messageModel;
    RxLogModel m_rxLogModel;

    // BLE scanning
    std::unique_ptr<QBluetoothDeviceDiscoveryAgent> m_bleDiscoveryAgent;
    bool m_scanning = false;
    QVariantList m_discoveredBleDevices;
    QList<QBluetoothDeviceInfo> m_discoveredBleDeviceInfos;

    // Internal state
    bool m_contactsSyncing = false;
    int m_channelQueryIndex = 0;
    bool m_queryingChannels = false;
    bool m_syncingMessages = false;
};

} // namespace MeshCore

#endif // MESHCOREDEVICE_H
