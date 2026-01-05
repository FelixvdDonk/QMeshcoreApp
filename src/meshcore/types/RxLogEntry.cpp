#include "RxLogEntry.h"

namespace MeshCore {

// Advert flags (from MeshCore protocol)
static constexpr quint8 ADV_LATLON_MASK = 0x10;
static constexpr quint8 ADV_BATTERY_MASK = 0x20;
static constexpr quint8 ADV_TEMPERATURE_MASK = 0x40;
static constexpr quint8 ADV_NAME_MASK = 0x80;

RxLogEntry::RxLogEntry(double snr, qint8 rssi, const QByteArray &rawData)
    : m_timestamp(QDateTime::currentDateTime())
    , m_snr(snr)
    , m_rssi(rssi)
    , m_rawData(rawData)
{
    parsePacket();
}

void RxLogEntry::parsePacket()
{
    if (m_rawData.isEmpty()) {
        return;
    }

    // Parse header byte
    quint8 header = static_cast<quint8>(m_rawData.at(0));
    m_routeType = header & 0x03;           // Bits 0-1
    m_payloadType = (header >> 2) & 0x0F;  // Bits 2-5
    m_payloadVersion = (header >> 6) & 0x03; // Bits 6-7

    // Determine offset based on route type (transport codes add 4 bytes)
    int offset = 1;
    bool hasTransportCodes = (m_routeType == RouteTransportFlood || m_routeType == RouteTransportDirect);
    if (hasTransportCodes) {
        offset += 4;  // Skip 2x 16-bit transport codes
    }

    // Parse path length if we have enough data
    if (m_rawData.size() > offset) {
        m_pathLength = static_cast<quint8>(m_rawData.at(offset));
        // Hop count is path_length / 6 (each hop is 6 bytes: public key prefix)
        m_hopCount = m_pathLength / 6;
        offset += 1;  // Skip path_len byte
    }

    // Skip path bytes
    offset += m_pathLength;

    // Extract payload
    if (m_rawData.size() > offset) {
        QByteArray payload = m_rawData.mid(offset);
        parsePayload(payload);
    }
}

void RxLogEntry::parsePayload(const QByteArray &payload)
{
    if (payload.isEmpty()) {
        return;
    }

    switch (m_payloadType) {
    case PayloadRequest:
    case PayloadResponse:
    case PayloadTextMsg:
    case PayloadAck:
    case PayloadGroupText:
    case PayloadGroupData:
    case PayloadPath:
        // These have dest/src hash prefix
        if (payload.size() >= 2) {
            m_destHash = static_cast<quint8>(payload.at(0));
            m_srcHash = static_cast<quint8>(payload.at(1));
        }
        break;
        
    case PayloadAdvert:
        parseAdvert(payload);
        break;
        
    case PayloadAnonRequest:
        // Has dest hash and 32-byte ephemeral public key
        if (payload.size() >= 1) {
            m_destHash = static_cast<quint8>(payload.at(0));
        }
        break;
        
    default:
        break;
    }
}

void RxLogEntry::parseAdvert(const QByteArray &payload)
{
    // Advert structure:
    // 32 bytes: public key
    // 4 bytes: timestamp
    // 64 bytes: signature
    // rest: app_data (flags + optional fields)
    
    if (payload.size() < 100) {  // 32 + 4 + 64 = 100 minimum
        return;
    }

    int offset = 32 + 4 + 64;  // Skip pub key, timestamp, signature
    
    if (payload.size() <= offset) {
        return;
    }

    quint8 flags = static_cast<quint8>(payload.at(offset));
    m_advertType = flags & 0x0F;
    offset++;

    // Parse lat/lon if present
    if ((flags & ADV_LATLON_MASK) && payload.size() >= offset + 8) {
        qint32 latRaw = static_cast<quint8>(payload.at(offset)) |
                       (static_cast<quint8>(payload.at(offset + 1)) << 8) |
                       (static_cast<quint8>(payload.at(offset + 2)) << 16) |
                       (static_cast<quint8>(payload.at(offset + 3)) << 24);
        qint32 lonRaw = static_cast<quint8>(payload.at(offset + 4)) |
                       (static_cast<quint8>(payload.at(offset + 5)) << 8) |
                       (static_cast<quint8>(payload.at(offset + 6)) << 16) |
                       (static_cast<quint8>(payload.at(offset + 7)) << 24);
        m_latitude = latRaw / 1e6;
        m_longitude = lonRaw / 1e6;
        m_hasLocation = true;
        offset += 8;
    }

    // Parse name if present (null-terminated string)
    if ((flags & ADV_NAME_MASK) && payload.size() > offset) {
        QByteArray nameBytes = payload.mid(offset);
        int nullPos = nameBytes.indexOf('\0');
        if (nullPos >= 0) {
            m_advertName = QString::fromUtf8(nameBytes.left(nullPos));
        } else {
            m_advertName = QString::fromUtf8(nameBytes);
        }
    }
}

QString RxLogEntry::rawDataHex() const
{
    return QString::fromLatin1(m_rawData.toHex(' ').toUpper());
}

QString RxLogEntry::routeTypeName() const
{
    switch (m_routeType) {
    case RouteTransportFlood: return QStringLiteral("T-FLOOD");
    case RouteFlood: return QStringLiteral("FLOOD");
    case RouteDirect: return QStringLiteral("DIRECT");
    case RouteTransportDirect: return QStringLiteral("T-DIRECT");
    default: return QStringLiteral("???");
    }
}

QString RxLogEntry::payloadTypeName() const
{
    switch (m_payloadType) {
    case PayloadRequest: return QStringLiteral("REQ");
    case PayloadResponse: return QStringLiteral("RESP");
    case PayloadTextMsg: return QStringLiteral("TEXT");
    case PayloadAck: return QStringLiteral("ACK");
    case PayloadAdvert: return QStringLiteral("ADVERT");
    case PayloadGroupText: return QStringLiteral("GRP_TXT");
    case PayloadGroupData: return QStringLiteral("GRP_DATA");
    case PayloadAnonRequest: return QStringLiteral("ANON_REQ");
    case PayloadPath: return QStringLiteral("PATH");
    case PayloadTrace: return QStringLiteral("TRACE");
    case PayloadMultipart: return QStringLiteral("MULTI");
    case PayloadControl: return QStringLiteral("CTRL");
    case PayloadRawCustom: return QStringLiteral("RAW");
    default: return QStringLiteral("???");
    }
}

QString RxLogEntry::advertTypeName() const
{
    switch (m_advertType) {
    case 0: return QStringLiteral("None");
    case 1: return QStringLiteral("Chat");
    case 2: return QStringLiteral("Repeater");
    case 3: return QStringLiteral("Room");
    default: return QStringLiteral("???");
    }
}

} // namespace MeshCore
