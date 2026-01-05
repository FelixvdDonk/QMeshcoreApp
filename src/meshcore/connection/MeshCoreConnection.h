#ifndef MESHCORECONNECTION_H
#define MESHCORECONNECTION_H

#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <functional>

#include "../MeshCoreConstants.h"
#include "../types/Contact.h"
#include "../types/SelfInfo.h"
#include "../types/DeviceInfo.h"
#include "../types/ChannelInfo.h"
#include "../types/ContactMessage.h"
#include "../types/ChannelMessage.h"
#include "../types/RepeaterStats.h"
#include "../types/TraceData.h"
#include "../types/TelemetryData.h"

namespace MeshCore {

class BufferWriter;

/**
 * @brief Abstract base class for MeshCore device connections
 *
 * Handles the protocol layer including command serialization and
 * response parsing. Subclasses implement the actual transport
 * (BLE or Serial).
 */
class MeshCoreConnection : public QObject
{
    Q_OBJECT

public:
    explicit MeshCoreConnection(QObject *parent = nullptr);
    ~MeshCoreConnection() override;

    // Connection state
    [[nodiscard]] bool isConnected() const { return m_connected; }

    // Abstract methods for subclasses
    virtual void close() = 0;

Q_SIGNALS:
    // Connection state
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

    // Raw frame signals
    void frameSent(const QByteArray &frame);
    void frameReceived(const QByteArray &frame);

    // Response signals
    void okResponse();
    void errorResponse(ErrorCode errorCode);
    void selfInfoReceived(const SelfInfo &selfInfo);
    void deviceInfoReceived(const DeviceInfo &deviceInfo);
    void contactsStarted(quint32 count);
    void contactReceived(const Contact &contact);
    void contactsEnded(quint32 mostRecentLastMod);
    void sentResponse(qint8 result, quint32 expectedAckCrc, quint32 estTimeout);
    void contactMessageReceived(const ContactMessage &message);
    void channelMessageReceived(const ChannelMessage &message);
    void noMoreMessages();
    void currentTimeReceived(quint32 epochSecs);
    void exportContactReceived(const QByteArray &advertPacketBytes);
    void batteryVoltageReceived(quint16 milliVolts);
    void privateKeyReceived(const QByteArray &privateKey);
    void disabledResponse();
    void channelInfoReceived(const ChannelInfo &channelInfo);
    void signStartReceived(quint32 maxSignDataLen);
    void signatureReceived(const QByteArray &signature);

    // Push notifications
    void advertPush(const QByteArray &publicKey);
    void newAdvertPush(const Contact &contact);
    void pathUpdatedPush(const QByteArray &publicKey);
    void sendConfirmedPush(quint32 ackCode, quint32 roundTrip);
    void msgWaitingPush();
    void rawDataPush(double snr, qint8 rssi, const QByteArray &payload);
    void loginSuccessPush(const QByteArray &pubKeyPrefix);
    void statusResponsePush(const QByteArray &pubKeyPrefix, const RepeaterStats &stats);
    void logRxDataPush(double snr, qint8 rssi, const QByteArray &raw);
    void telemetryResponsePush(const TelemetryData &telemetry);
    void traceDataPush(const TraceData &traceData);
    void binaryResponsePush(quint32 tag, const QByteArray &responseData);

public Q_SLOTS:
    // Low-level command methods (send raw commands)
    void sendCommandAppStart(const QString &appName = QStringLiteral("QMeshCore"));
    void sendCommandSendTxtMsg(TxtType txtType, quint8 attempt, quint32 senderTimestamp,
                               const QByteArray &pubKeyPrefix, const QString &text);
    void sendCommandSendChannelTxtMsg(TxtType txtType, quint8 channelIdx,
                                      quint32 senderTimestamp, const QString &text);
    void sendCommandGetContacts(quint32 since = 0);
    void sendCommandGetDeviceTime();
    void sendCommandSetDeviceTime(quint32 epochSecs);
    void sendCommandSendSelfAdvert(SelfAdvertType type);
    void sendCommandSetAdvertName(const QString &name);
    void sendCommandAddUpdateContact(const QByteArray &publicKey, AdvertType type, quint8 flags,
                                     qint8 outPathLen, const QByteArray &outPath,
                                     const QString &advName, quint32 lastAdvert,
                                     qint32 advLat, qint32 advLon);
    void sendCommandSyncNextMessage();
    void sendCommandSetRadioParams(quint32 radioFreq, quint32 radioBw, quint8 radioSf, quint8 radioCr);
    void sendCommandSetTxPower(quint8 txPower);
    void sendCommandResetPath(const QByteArray &pubKey);
    void sendCommandSetAdvertLatLon(qint32 lat, qint32 lon);
    void sendCommandRemoveContact(const QByteArray &pubKey);
    void sendCommandShareContact(const QByteArray &pubKey);
    void sendCommandExportContact(const QByteArray &pubKey = QByteArray());
    void sendCommandImportContact(const QByteArray &advertPacketBytes);
    void sendCommandReboot();
    void sendCommandGetBatteryVoltage();
    void sendCommandDeviceQuery(quint8 appTargetVer);
    void sendCommandExportPrivateKey();
    void sendCommandImportPrivateKey(const QByteArray &privateKey);
    void sendCommandSendRawData(const QByteArray &path, const QByteArray &rawData);
    void sendCommandSendLogin(const QByteArray &publicKey, const QString &password);
    void sendCommandSendStatusReq(const QByteArray &publicKey);
    void sendCommandSendTelemetryReq(const QByteArray &publicKey);
    void sendCommandSendBinaryReq(const QByteArray &publicKey, const QByteArray &requestCodeAndParams);
    void sendCommandGetChannel(quint8 channelIdx);
    void sendCommandSetChannel(quint8 channelIdx, const QString &name, const QByteArray &secret);
    void sendCommandSignStart();
    void sendCommandSignData(const QByteArray &dataToSign);
    void sendCommandSignFinish();
    void sendCommandSendTracePath(quint32 tag, quint32 auth, const QByteArray &path);
    void sendCommandSetOtherParams(bool manualAddContacts);

protected:
    // For subclasses to implement
    virtual void sendToRadioFrame(const QByteArray &frame) = 0;

    // Call this when connected
    void onConnected();
    void onDisconnected();

    // Call this when a frame is received from the device
    void onFrameReceived(const QByteArray &frame);

    bool m_connected = false;

private:
    // Response handlers
    void handleOkResponse(class BufferReader &reader);
    void handleErrorResponse(class BufferReader &reader);
    void handleSelfInfoResponse(class BufferReader &reader);
    void handleDeviceInfoResponse(class BufferReader &reader);
    void handleContactsStartResponse(class BufferReader &reader);
    void handleContactResponse(class BufferReader &reader);
    void handleEndOfContactsResponse(class BufferReader &reader);
    void handleSentResponse(class BufferReader &reader);
    void handleContactMsgRecvResponse(class BufferReader &reader);
    void handleChannelMsgRecvResponse(class BufferReader &reader);
    void handleNoMoreMessagesResponse(class BufferReader &reader);
    void handleCurrentTimeResponse(class BufferReader &reader);
    void handleExportContactResponse(class BufferReader &reader);
    void handleBatteryVoltageResponse(class BufferReader &reader);
    void handlePrivateKeyResponse(class BufferReader &reader);
    void handleDisabledResponse(class BufferReader &reader);
    void handleChannelInfoResponse(class BufferReader &reader);
    void handleSignStartResponse(class BufferReader &reader);
    void handleSignatureResponse(class BufferReader &reader);

    // Push handlers
    void handleAdvertPush(class BufferReader &reader);
    void handleNewAdvertPush(class BufferReader &reader);
    void handlePathUpdatedPush(class BufferReader &reader);
    void handleSendConfirmedPush(class BufferReader &reader);
    void handleMsgWaitingPush(class BufferReader &reader);
    void handleRawDataPush(class BufferReader &reader);
    void handleLoginSuccessPush(class BufferReader &reader);
    void handleStatusResponsePush(class BufferReader &reader);
    void handleLogRxDataPush(class BufferReader &reader);
    void handleTelemetryResponsePush(class BufferReader &reader);
    void handleTraceDataPush(class BufferReader &reader);
    void handleBinaryResponsePush(class BufferReader &reader);
};

} // namespace MeshCore

#endif // MESHCORECONNECTION_H
