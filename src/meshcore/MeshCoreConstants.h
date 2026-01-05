#ifndef MESHCORECONSTANTS_H
#define MESHCORECONSTANTS_H

#include <QObject>
#include <QBluetoothUuid>

namespace MeshCore {

Q_NAMESPACE

// Supported companion protocol version (3 = latest, supports V3 message format)
inline constexpr int SupportedCompanionProtocolVersion = 3;

// Serial frame types
namespace SerialFrameTypes {
    inline constexpr quint8 Incoming = 0x3e; // ">"
    inline constexpr quint8 Outgoing = 0x3c; // "<"
}

// BLE UUIDs
namespace Ble {
    inline const QBluetoothUuid ServiceUuid{
        QStringLiteral("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")};
    inline const QBluetoothUuid CharacteristicUuidRx{
        QStringLiteral("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")};
    inline const QBluetoothUuid CharacteristicUuidTx{
        QStringLiteral("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")};
}

// Command codes sent to device
enum class CommandCode : quint8 {
    AppStart = 1,
    SendTxtMsg = 2,
    SendChannelTxtMsg = 3,
    GetContacts = 4,
    GetDeviceTime = 5,
    SetDeviceTime = 6,
    SendSelfAdvert = 7,
    SetAdvertName = 8,
    AddUpdateContact = 9,
    SyncNextMessage = 10,
    SetRadioParams = 11,
    SetTxPower = 12,
    ResetPath = 13,
    SetAdvertLatLon = 14,
    RemoveContact = 15,
    ShareContact = 16,
    ExportContact = 17,
    ImportContact = 18,
    Reboot = 19,
    GetBatteryVoltage = 20,
    SetTuningParams = 21,
    DeviceQuery = 22,
    ExportPrivateKey = 23,
    ImportPrivateKey = 24,
    SendRawData = 25,
    SendLogin = 26,
    SendStatusReq = 27,
    GetChannel = 31,
    SetChannel = 32,
    SignStart = 33,
    SignData = 34,
    SignFinish = 35,
    SendTracePath = 36,
    SetOtherParams = 38,
    SendTelemetryReq = 39,
    SendBinaryReq = 50
};
Q_ENUM_NS(CommandCode)

// Response codes received from device
enum class ResponseCode : quint8 {
    Ok = 0,
    Err = 1,
    ContactsStart = 2,
    Contact = 3,
    EndOfContacts = 4,
    SelfInfo = 5,
    Sent = 6,
    ContactMsgRecv = 7,
    ChannelMsgRecv = 8,
    CurrTime = 9,
    NoMoreMessages = 10,
    ExportContact = 11,
    BatteryVoltage = 12,
    DeviceInfo = 13,
    PrivateKey = 14,
    Disabled = 15,
    ChannelInfo = 18,
    SignStart = 19,
    Signature = 20
};
Q_ENUM_NS(ResponseCode)

// Push codes (unsolicited notifications from device)
enum class PushCode : quint8 {
    Advert = 0x80,
    PathUpdated = 0x81,
    SendConfirmed = 0x82,
    MsgWaiting = 0x83,
    RawData = 0x84,
    LoginSuccess = 0x85,
    LoginFail = 0x86,
    StatusResponse = 0x87,
    LogRxData = 0x88,
    TraceData = 0x89,
    NewAdvert = 0x8A,
    TelemetryResponse = 0x8B,
    BinaryResponse = 0x8C
};
Q_ENUM_NS(PushCode)

// Error codes
enum class ErrorCode : quint8 {
    UnsupportedCmd = 1,
    NotFound = 2,
    TableFull = 3,
    BadState = 4,
    FileIoError = 5,
    IllegalArg = 6
};
Q_ENUM_NS(ErrorCode)

// Advert types
enum class AdvertType : quint8 {
    None = 0,
    Chat = 1,
    Repeater = 2,
    Room = 3
};
Q_ENUM_NS(AdvertType)

// Self advert types (for sending adverts)
enum class SelfAdvertType : quint8 {
    ZeroHop = 0,
    Flood = 1
};
Q_ENUM_NS(SelfAdvertType)

// Text message types
enum class TxtType : quint8 {
    Plain = 0,
    CliData = 1,
    SignedPlain = 2
};
Q_ENUM_NS(TxtType)

// Binary request types
enum class BinaryRequestType : quint8 {
    GetTelemetryData = 0x03,
    GetAvgMinMax = 0x04,
    GetAccessList = 0x05,
    GetNeighbours = 0x06
};
Q_ENUM_NS(BinaryRequestType)

// Packet header masks and shifts
namespace PacketHeader {
    inline constexpr quint8 RouteMask = 0x03;
    inline constexpr quint8 TypeShift = 2;
    inline constexpr quint8 TypeMask = 0x0F;
    inline constexpr quint8 VerShift = 6;
    inline constexpr quint8 VerMask = 0x03;
}

// Route types
enum class RouteType : quint8 {
    Reserved1 = 0x00,
    Flood = 0x01,
    Direct = 0x02,
    Reserved2 = 0x03
};
Q_ENUM_NS(RouteType)

// Payload types
enum class PayloadType : quint8 {
    Req = 0x00,
    Response = 0x01,
    TxtMsg = 0x02,
    Ack = 0x03,
    Advert = 0x04,
    GrpTxt = 0x05,
    GrpData = 0x06,
    AnonReq = 0x07,
    Path = 0x08,
    Trace = 0x09,
    RawCustom = 0x0F
};
Q_ENUM_NS(PayloadType)

// Advert flags
namespace AdvertFlags {
    inline constexpr quint8 LatLonMask = 0x10;
    inline constexpr quint8 BatteryMask = 0x20;
    inline constexpr quint8 TemperatureMask = 0x40;
    inline constexpr quint8 NameMask = 0x80;
}

// Connection type for MeshCoreDevice
enum class ConnectionType {
    None,
    Ble,
    Serial
};
Q_ENUM_NS(ConnectionType)

// Connection state
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Error
};
Q_ENUM_NS(ConnectionState)

// Neighbour order by options
enum class NeighbourOrderBy : quint8 {
    NewestToOldest = 0,
    OldestToNewest = 1,
    StrongestToWeakest = 2,
    WeakestToStrongest = 3
};
Q_ENUM_NS(NeighbourOrderBy)

} // namespace MeshCore

#endif // MESHCORECONSTANTS_H
