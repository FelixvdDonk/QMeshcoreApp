#include "MeshCoreConnection.h"
#include "../utils/BufferReader.h"
#include "../utils/BufferWriter.h"
#include <QTimer>

namespace MeshCore {

MeshCoreConnection::MeshCoreConnection(QObject *parent)
    : QObject(parent)
{
}

MeshCoreConnection::~MeshCoreConnection() = default;

void MeshCoreConnection::onConnected()
{
    m_connected = true;

    // Send device query immediately - connection is already stable at this point
    qDebug() << "Sending DeviceQuery with protocol version" << SupportedCompanionProtocolVersion;
    sendCommandDeviceQuery(SupportedCompanionProtocolVersion);

    Q_EMIT connected();
}

void MeshCoreConnection::onDisconnected()
{
    m_connected = false;
    Q_EMIT disconnected();
}

void MeshCoreConnection::onFrameReceived(const QByteArray &frame)
{
    qDebug() << "Frame received:" << frame.size() << "bytes, data:" << frame.toHex();
    Q_EMIT frameReceived(frame);

    if (frame.isEmpty()) {
        return;
    }

    BufferReader reader(frame);
    quint8 responseCode = reader.readByte();

    // Response codes
    if (responseCode == static_cast<quint8>(ResponseCode::Ok)) {
        handleOkResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::Err)) {
        handleErrorResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::SelfInfo)) {
        handleSelfInfoResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::DeviceInfo)) {
        handleDeviceInfoResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::ContactsStart)) {
        handleContactsStartResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::Contact)) {
        handleContactResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::EndOfContacts)) {
        handleEndOfContactsResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::Sent)) {
        handleSentResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::ContactMsgRecv)) {
        handleContactMsgRecvResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::ChannelMsgRecv)) {
        handleChannelMsgRecvResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::NoMoreMessages)) {
        handleNoMoreMessagesResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::CurrTime)) {
        handleCurrentTimeResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::ExportContact)) {
        handleExportContactResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::BatteryVoltage)) {
        handleBatteryVoltageResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::PrivateKey)) {
        handlePrivateKeyResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::Disabled)) {
        handleDisabledResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::ChannelInfo)) {
        handleChannelInfoResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::SignStart)) {
        handleSignStartResponse(reader);
    } else if (responseCode == static_cast<quint8>(ResponseCode::Signature)) {
        handleSignatureResponse(reader);
    }
    // Push codes
    else if (responseCode == static_cast<quint8>(PushCode::Advert)) {
        handleAdvertPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::NewAdvert)) {
        handleNewAdvertPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::PathUpdated)) {
        handlePathUpdatedPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::SendConfirmed)) {
        handleSendConfirmedPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::MsgWaiting)) {
        handleMsgWaitingPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::RawData)) {
        handleRawDataPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::LoginSuccess)) {
        handleLoginSuccessPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::StatusResponse)) {
        handleStatusResponsePush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::LogRxData)) {
        handleLogRxDataPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::TelemetryResponse)) {
        handleTelemetryResponsePush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::TraceData)) {
        handleTraceDataPush(reader);
    } else if (responseCode == static_cast<quint8>(PushCode::BinaryResponse)) {
        handleBinaryResponsePush(reader);
    } else {
        qWarning() << "Unhandled frame code:" << responseCode;
    }
}

// Response handlers
void MeshCoreConnection::handleOkResponse(BufferReader &)
{
    Q_EMIT okResponse();
}

void MeshCoreConnection::handleErrorResponse(BufferReader &reader)
{
    ErrorCode errCode = ErrorCode::UnsupportedCmd;
    if (reader.hasRemaining()) {
        errCode = static_cast<ErrorCode>(reader.readByte());
    }
    Q_EMIT errorResponse(errCode);
}

void MeshCoreConnection::handleSelfInfoResponse(BufferReader &reader)
{
    AdvertType type = static_cast<AdvertType>(reader.readByte());
    quint8 txPower = reader.readByte();
    quint8 maxTxPower = reader.readByte();
    QByteArray publicKey = reader.readBytes(32);
    qint32 advLat = reader.readInt32LE();
    qint32 advLon = reader.readInt32LE();
    reader.skip(3);  // reserved
    bool manualAddContacts = reader.readByte() != 0;
    quint32 radioFreq = reader.readUInt32LE();
    quint32 radioBw = reader.readUInt32LE();
    quint8 radioSf = reader.readByte();
    quint8 radioCr = reader.readByte();
    QString name = reader.readString();

    SelfInfo selfInfo(type, txPower, maxTxPower, publicKey, advLat, advLon,
                      manualAddContacts, radioFreq, radioBw, radioSf, radioCr, name);
    Q_EMIT selfInfoReceived(selfInfo);
}

void MeshCoreConnection::handleDeviceInfoResponse(BufferReader &reader)
{
    qint8 firmwareVer = reader.readInt8();
    reader.skip(6);  // reserved
    QString firmwareBuildDate = reader.readCString(12);
    QString manufacturerModel = reader.readString();

    DeviceInfo info(firmwareVer, firmwareBuildDate, manufacturerModel);
    Q_EMIT deviceInfoReceived(info);
}

void MeshCoreConnection::handleContactsStartResponse(BufferReader &reader)
{
    quint32 count = reader.readUInt32LE();
    Q_EMIT contactsStarted(count);
}

void MeshCoreConnection::handleContactResponse(BufferReader &reader)
{
    QByteArray publicKey = reader.readBytes(32);
    AdvertType type = static_cast<AdvertType>(reader.readByte());
    quint8 flags = reader.readByte();
    qint8 outPathLen = reader.readInt8();
    QByteArray outPath = reader.readBytes(64);
    QString advName = reader.readCString(32);
    quint32 lastAdvert = reader.readUInt32LE();
    qint32 advLat = reader.readInt32LE();  // Signed - can be negative for southern hemisphere
    qint32 advLon = reader.readInt32LE();  // Signed - can be negative for western hemisphere
    quint32 lastMod = reader.readUInt32LE();

    Contact contact(publicKey, type, flags, outPathLen, outPath, advName,
                    lastAdvert, advLat, advLon, lastMod);
    Q_EMIT contactReceived(contact);
}

void MeshCoreConnection::handleEndOfContactsResponse(BufferReader &reader)
{
    quint32 mostRecentLastMod = reader.readUInt32LE();
    Q_EMIT contactsEnded(mostRecentLastMod);
}

void MeshCoreConnection::handleSentResponse(BufferReader &reader)
{
    qint8 result = reader.readInt8();
    quint32 expectedAckCrc = reader.readUInt32LE();
    quint32 estTimeout = reader.readUInt32LE();
    Q_EMIT sentResponse(result, expectedAckCrc, estTimeout);
}

void MeshCoreConnection::handleContactMsgRecvResponse(BufferReader &reader)
{
    QByteArray pubKeyPrefix = reader.readBytes(6);
    quint8 pathLen = reader.readByte();
    TxtType txtType = static_cast<TxtType>(reader.readByte());
    quint32 senderTimestamp = reader.readUInt32LE();
    QString text = reader.readString();

    ContactMessage msg(pubKeyPrefix, pathLen, txtType, senderTimestamp, text);
    Q_EMIT contactMessageReceived(msg);
}

void MeshCoreConnection::handleChannelMsgRecvResponse(BufferReader &reader)
{
    qint8 channelIdx = reader.readInt8();
    quint8 pathLen = reader.readByte();
    TxtType txtType = static_cast<TxtType>(reader.readByte());
    quint32 senderTimestamp = reader.readUInt32LE();
    QString text = reader.readString();

    ChannelMessage msg(channelIdx, pathLen, txtType, senderTimestamp, text);
    Q_EMIT channelMessageReceived(msg);
}

void MeshCoreConnection::handleNoMoreMessagesResponse(BufferReader &)
{
    Q_EMIT noMoreMessages();
}

void MeshCoreConnection::handleCurrentTimeResponse(BufferReader &reader)
{
    quint32 epochSecs = reader.readUInt32LE();
    Q_EMIT currentTimeReceived(epochSecs);
}

void MeshCoreConnection::handleExportContactResponse(BufferReader &reader)
{
    QByteArray advertPacketBytes = reader.readRemainingBytes();
    Q_EMIT exportContactReceived(advertPacketBytes);
}

void MeshCoreConnection::handleBatteryVoltageResponse(BufferReader &reader)
{
    quint16 milliVolts = reader.readUInt16LE();
    Q_EMIT batteryVoltageReceived(milliVolts);
}

void MeshCoreConnection::handlePrivateKeyResponse(BufferReader &reader)
{
    QByteArray privateKey = reader.readBytes(64);
    Q_EMIT privateKeyReceived(privateKey);
}

void MeshCoreConnection::handleDisabledResponse(BufferReader &)
{
    Q_EMIT disabledResponse();
}

void MeshCoreConnection::handleChannelInfoResponse(BufferReader &reader)
{
    quint8 idx = reader.readUInt8();
    QString name = reader.readCString(32);

    qsizetype remaining = reader.remainingBytes();
    QByteArray secret;
    if (remaining == 16) {
        secret = reader.readBytes(16);
    }

    ChannelInfo info(idx, name, secret);
    Q_EMIT channelInfoReceived(info);
}

void MeshCoreConnection::handleSignStartResponse(BufferReader &reader)
{
    reader.skip(1);  // reserved
    quint32 maxSignDataLen = reader.readUInt32LE();
    Q_EMIT signStartReceived(maxSignDataLen);
}

void MeshCoreConnection::handleSignatureResponse(BufferReader &reader)
{
    QByteArray signature = reader.readBytes(64);
    Q_EMIT signatureReceived(signature);
}

// Push handlers
void MeshCoreConnection::handleAdvertPush(BufferReader &reader)
{
    QByteArray publicKey = reader.readBytes(32);
    Q_EMIT advertPush(publicKey);
}

void MeshCoreConnection::handleNewAdvertPush(BufferReader &reader)
{
    QByteArray publicKey = reader.readBytes(32);
    AdvertType type = static_cast<AdvertType>(reader.readByte());
    quint8 flags = reader.readByte();
    qint8 outPathLen = reader.readInt8();
    QByteArray outPath = reader.readBytes(64);
    QString advName = reader.readCString(32);
    quint32 lastAdvert = reader.readUInt32LE();
    qint32 advLat = reader.readUInt32LE();
    qint32 advLon = reader.readUInt32LE();
    quint32 lastMod = reader.readUInt32LE();

    Contact contact(publicKey, type, flags, outPathLen, outPath, advName,
                    lastAdvert, advLat, advLon, lastMod);
    Q_EMIT newAdvertPush(contact);
}

void MeshCoreConnection::handlePathUpdatedPush(BufferReader &reader)
{
    QByteArray publicKey = reader.readBytes(32);
    Q_EMIT pathUpdatedPush(publicKey);
}

void MeshCoreConnection::handleSendConfirmedPush(BufferReader &reader)
{
    quint32 ackCode = reader.readUInt32LE();
    quint32 roundTrip = reader.readUInt32LE();
    Q_EMIT sendConfirmedPush(ackCode, roundTrip);
}

void MeshCoreConnection::handleMsgWaitingPush(BufferReader &)
{
    Q_EMIT msgWaitingPush();
}

void MeshCoreConnection::handleRawDataPush(BufferReader &reader)
{
    double snr = reader.readInt8() / 4.0;
    qint8 rssi = reader.readInt8();
    reader.skip(1);  // reserved
    QByteArray payload = reader.readRemainingBytes();
    Q_EMIT rawDataPush(snr, rssi, payload);
}

void MeshCoreConnection::handleLoginSuccessPush(BufferReader &reader)
{
    reader.skip(1);  // reserved
    QByteArray pubKeyPrefix = reader.readBytes(6);
    Q_EMIT loginSuccessPush(pubKeyPrefix);
}

void MeshCoreConnection::handleStatusResponsePush(BufferReader &reader)
{
    reader.skip(1);  // reserved
    QByteArray pubKeyPrefix = reader.readBytes(6);
    QByteArray statusData = reader.readRemainingBytes();

    // Parse repeater stats from status data
    BufferReader statsReader(statusData);
    quint16 battMv = statsReader.readUInt16LE();
    quint16 txQueueLen = statsReader.readUInt16LE();
    qint16 noiseFloor = statsReader.readInt16LE();
    qint16 lastRssi = statsReader.readInt16LE();
    quint32 packetsRecv = statsReader.readUInt32LE();
    quint32 packetsSent = statsReader.readUInt32LE();
    quint32 totalAirTime = statsReader.readUInt32LE();
    quint32 totalUpTime = statsReader.readUInt32LE();
    quint32 sentFlood = statsReader.readUInt32LE();
    quint32 sentDirect = statsReader.readUInt32LE();
    quint32 recvFlood = statsReader.readUInt32LE();
    quint32 recvDirect = statsReader.readUInt32LE();
    quint16 errEvents = statsReader.readUInt16LE();
    qint16 lastSnr = statsReader.readInt16LE();
    quint16 directDups = statsReader.readUInt16LE();
    quint16 floodDups = statsReader.readUInt16LE();

    RepeaterStats stats(battMv, txQueueLen, noiseFloor, lastRssi, packetsRecv, packetsSent,
                        totalAirTime, totalUpTime, sentFlood, sentDirect, recvFlood, recvDirect,
                        errEvents, lastSnr, directDups, floodDups);
    Q_EMIT statusResponsePush(pubKeyPrefix, stats);
}

void MeshCoreConnection::handleLogRxDataPush(BufferReader &reader)
{
    double snr = reader.readInt8() / 4.0;
    qint8 rssi = reader.readInt8();
    QByteArray raw = reader.readRemainingBytes();
    Q_EMIT logRxDataPush(snr, rssi, raw);
}

void MeshCoreConnection::handleTelemetryResponsePush(BufferReader &reader)
{
    reader.skip(1);  // reserved
    QByteArray pubKeyPrefix = reader.readBytes(6);
    QByteArray lppData = reader.readRemainingBytes();

    TelemetryData telemetry = TelemetryData::fromLppData(pubKeyPrefix, lppData);
    Q_EMIT telemetryResponsePush(telemetry);
}

void MeshCoreConnection::handleTraceDataPush(BufferReader &reader)
{
    reader.skip(1);  // reserved
    quint8 pathLen = reader.readUInt8();
    quint8 flags = reader.readUInt8();
    quint32 tag = reader.readUInt32LE();
    quint32 authCode = reader.readUInt32LE();
    QByteArray pathHashes = reader.readBytes(pathLen);
    QByteArray pathSnrs = reader.readBytes(pathLen);
    qint8 lastSnr = reader.readInt8();

    TraceData trace(pathLen, flags, tag, authCode, pathHashes, pathSnrs, lastSnr);
    Q_EMIT traceDataPush(trace);
}

void MeshCoreConnection::handleBinaryResponsePush(BufferReader &reader)
{
    reader.skip(1);  // reserved
    quint32 tag = reader.readUInt32LE();
    QByteArray responseData = reader.readRemainingBytes();
    Q_EMIT binaryResponsePush(tag, responseData);
}

// Command implementations
void MeshCoreConnection::sendCommandAppStart(const QString &appName)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::AppStart));
    writer.writeByte(1);  // appVer
    writer.writeBytes(QByteArray(6, '\0'));  // reserved
    writer.writeString(appName);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendTxtMsg(TxtType txtType, quint8 attempt,
                                                quint32 senderTimestamp,
                                                const QByteArray &pubKeyPrefix,
                                                const QString &text)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendTxtMsg));
    writer.writeByte(static_cast<quint8>(txtType));
    writer.writeByte(attempt);
    writer.writeUInt32LE(senderTimestamp);
    writer.writeBytes(pubKeyPrefix.left(6));
    writer.writeString(text);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendChannelTxtMsg(TxtType txtType, quint8 channelIdx,
                                                       quint32 senderTimestamp,
                                                       const QString &text)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendChannelTxtMsg));
    writer.writeByte(static_cast<quint8>(txtType));
    writer.writeByte(channelIdx);
    writer.writeUInt32LE(senderTimestamp);
    writer.writeString(text);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandGetContacts(quint32 since)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::GetContacts));
    if (since > 0) {
        writer.writeUInt32LE(since);
    }
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandGetDeviceTime()
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::GetDeviceTime));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSetDeviceTime(quint32 epochSecs)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SetDeviceTime));
    writer.writeUInt32LE(epochSecs);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendSelfAdvert(SelfAdvertType type)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendSelfAdvert));
    writer.writeByte(static_cast<quint8>(type));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSetAdvertName(const QString &name)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SetAdvertName));
    writer.writeString(name);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandAddUpdateContact(const QByteArray &publicKey,
                                                      AdvertType type, quint8 flags,
                                                      qint8 outPathLen,
                                                      const QByteArray &outPath,
                                                      const QString &advName,
                                                      quint32 lastAdvert,
                                                      qint32 advLat, qint32 advLon)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::AddUpdateContact));
    writer.writeBytes(publicKey);
    writer.writeByte(static_cast<quint8>(type));
    writer.writeByte(flags);
    writer.writeInt8(outPathLen);

    // Ensure outPath is 64 bytes
    QByteArray paddedPath = outPath;
    paddedPath.resize(64);
    writer.writeBytes(paddedPath);

    writer.writeCString(advName, 32);
    writer.writeUInt32LE(lastAdvert);
    writer.writeUInt32LE(static_cast<quint32>(advLat));
    writer.writeUInt32LE(static_cast<quint32>(advLon));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSyncNextMessage()
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SyncNextMessage));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSetRadioParams(quint32 radioFreq, quint32 radioBw,
                                                    quint8 radioSf, quint8 radioCr)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SetRadioParams));
    writer.writeUInt32LE(radioFreq);
    writer.writeUInt32LE(radioBw);
    writer.writeByte(radioSf);
    writer.writeByte(radioCr);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSetTxPower(quint8 txPower)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SetTxPower));
    writer.writeByte(txPower);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandResetPath(const QByteArray &pubKey)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::ResetPath));
    writer.writeBytes(pubKey);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSetAdvertLatLon(qint32 lat, qint32 lon)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SetAdvertLatLon));
    writer.writeInt32LE(lat);
    writer.writeInt32LE(lon);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandRemoveContact(const QByteArray &pubKey)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::RemoveContact));
    writer.writeBytes(pubKey);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandShareContact(const QByteArray &pubKey)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::ShareContact));
    writer.writeBytes(pubKey);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandExportContact(const QByteArray &pubKey)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::ExportContact));
    if (!pubKey.isEmpty()) {
        writer.writeBytes(pubKey);
    }
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandImportContact(const QByteArray &advertPacketBytes)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::ImportContact));
    writer.writeBytes(advertPacketBytes);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandReboot()
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::Reboot));
    writer.writeString(QStringLiteral("reboot"));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandGetBatteryVoltage()
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::GetBatteryVoltage));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandDeviceQuery(quint8 appTargetVer)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::DeviceQuery));
    writer.writeByte(appTargetVer);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandExportPrivateKey()
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::ExportPrivateKey));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandImportPrivateKey(const QByteArray &privateKey)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::ImportPrivateKey));
    writer.writeBytes(privateKey);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendRawData(const QByteArray &path, const QByteArray &rawData)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendRawData));
    writer.writeByte(static_cast<quint8>(path.size()));
    writer.writeBytes(path);
    writer.writeBytes(rawData);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendLogin(const QByteArray &publicKey, const QString &password)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendLogin));
    writer.writeBytes(publicKey);
    writer.writeString(password.left(15));  // max 15 chars
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendStatusReq(const QByteArray &publicKey)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendStatusReq));
    writer.writeBytes(publicKey);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendTelemetryReq(const QByteArray &publicKey)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendTelemetryReq));
    writer.writeByte(0);  // reserved
    writer.writeByte(0);  // reserved
    writer.writeByte(0);  // reserved
    writer.writeBytes(publicKey);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendBinaryReq(const QByteArray &publicKey,
                                                   const QByteArray &requestCodeAndParams)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendBinaryReq));
    writer.writeBytes(publicKey);
    writer.writeBytes(requestCodeAndParams);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandGetChannel(quint8 channelIdx)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::GetChannel));
    writer.writeByte(channelIdx);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSetChannel(quint8 channelIdx, const QString &name,
                                                const QByteArray &secret)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SetChannel));
    writer.writeByte(channelIdx);
    writer.writeCString(name, 32);
    writer.writeBytes(secret);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSignStart()
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SignStart));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSignData(const QByteArray &dataToSign)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SignData));
    writer.writeBytes(dataToSign);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSignFinish()
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SignFinish));
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSendTracePath(quint32 tag, quint32 auth, const QByteArray &path)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SendTracePath));
    writer.writeUInt32LE(tag);
    writer.writeUInt32LE(auth);
    writer.writeByte(0);  // flags
    writer.writeBytes(path);
    sendToRadioFrame(writer.toByteArray());
}

void MeshCoreConnection::sendCommandSetOtherParams(bool manualAddContacts)
{
    BufferWriter writer;
    writer.writeByte(static_cast<quint8>(CommandCode::SetOtherParams));
    writer.writeByte(manualAddContacts ? 1 : 0);
    sendToRadioFrame(writer.toByteArray());
}

} // namespace MeshCore
