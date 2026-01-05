#include "Packet.h"
#include "Advert.h"
#include "../utils/BufferReader.h"

namespace MeshCore {

Packet::Packet(quint8 header, const QByteArray &path, const QByteArray &payload)
    : m_header(header)
    , m_path(path)
    , m_payload(payload)
{
}

Packet Packet::fromBytes(const QByteArray &data)
{
    BufferReader reader(data);

    quint8 header = reader.readByte();
    qint8 pathLen = reader.readInt8();
    QByteArray path = reader.readBytes(pathLen);
    QByteArray payload = reader.readRemainingBytes();

    return Packet(header, path, payload);
}

RouteType Packet::routeType() const
{
    return static_cast<RouteType>(m_header & PacketHeader::RouteMask);
}

QString Packet::routeTypeString() const
{
    switch (routeType()) {
    case RouteType::Flood: return QStringLiteral("FLOOD");
    case RouteType::Direct: return QStringLiteral("DIRECT");
    case RouteType::Reserved1: return QStringLiteral("RESERVED1");
    case RouteType::Reserved2: return QStringLiteral("RESERVED2");
    default: return QStringLiteral("UNKNOWN");
    }
}

PayloadType Packet::payloadType() const
{
    return static_cast<PayloadType>((m_header >> PacketHeader::TypeShift) & PacketHeader::TypeMask);
}

QString Packet::payloadTypeString() const
{
    switch (payloadType()) {
    case PayloadType::Req: return QStringLiteral("REQ");
    case PayloadType::Response: return QStringLiteral("RESPONSE");
    case PayloadType::TxtMsg: return QStringLiteral("TXT_MSG");
    case PayloadType::Ack: return QStringLiteral("ACK");
    case PayloadType::Advert: return QStringLiteral("ADVERT");
    case PayloadType::GrpTxt: return QStringLiteral("GRP_TXT");
    case PayloadType::GrpData: return QStringLiteral("GRP_DATA");
    case PayloadType::AnonReq: return QStringLiteral("ANON_REQ");
    case PayloadType::Path: return QStringLiteral("PATH");
    case PayloadType::Trace: return QStringLiteral("TRACE");
    case PayloadType::RawCustom: return QStringLiteral("RAW_CUSTOM");
    default: return QStringLiteral("UNKNOWN");
    }
}

quint8 Packet::payloadVersion() const
{
    return (m_header >> PacketHeader::VerShift) & PacketHeader::VerMask;
}

Advert Packet::parseAdvertPayload() const
{
    if (payloadType() != PayloadType::Advert) {
        return Advert();
    }
    return Advert::fromBytes(m_payload);
}

} // namespace MeshCore
