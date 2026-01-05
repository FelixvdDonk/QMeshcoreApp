#include "Advert.h"
#include "../utils/BufferReader.h"

namespace MeshCore {

Advert::Advert(const QByteArray &publicKey, quint32 timestamp,
               const QByteArray &signature, const QByteArray &appData)
    : m_publicKey(publicKey)
    , m_timestamp(timestamp)
    , m_signature(signature)
    , m_appData(appData)
{
    parseAppData();
}

Advert Advert::fromBytes(const QByteArray &data)
{
    BufferReader reader(data);

    QByteArray publicKey = reader.readBytes(32);
    quint32 timestamp = reader.readUInt32LE();
    QByteArray signature = reader.readBytes(64);
    QByteArray appData = reader.readRemainingBytes();

    return Advert(publicKey, timestamp, signature, appData);
}

QString Advert::publicKeyHex() const
{
    return QString::fromLatin1(m_publicKey.toHex());
}

quint8 Advert::flags() const
{
    return m_flags;
}

AdvertType Advert::type() const
{
    return static_cast<AdvertType>(m_flags & 0x0F);
}

QString Advert::typeString() const
{
    switch (type()) {
    case AdvertType::None: return QStringLiteral("None");
    case AdvertType::Chat: return QStringLiteral("Chat");
    case AdvertType::Repeater: return QStringLiteral("Repeater");
    case AdvertType::Room: return QStringLiteral("Room");
    default: return QStringLiteral("Unknown");
    }
}

bool Advert::hasLatLon() const
{
    return m_flags & AdvertFlags::LatLonMask;
}

bool Advert::hasName() const
{
    return m_flags & AdvertFlags::NameMask;
}

double Advert::latitudeDecimal() const
{
    return static_cast<double>(m_latitude) / 1e7;
}

double Advert::longitudeDecimal() const
{
    return static_cast<double>(m_longitude) / 1e7;
}

void Advert::parseAppData()
{
    if (m_appData.isEmpty()) {
        return;
    }

    BufferReader reader(m_appData);
    m_flags = reader.readByte();

    // Parse lat/lon if present
    if (m_flags & AdvertFlags::LatLonMask) {
        if (reader.remainingBytes() >= 8) {
            m_latitude = reader.readInt32LE();
            m_longitude = reader.readInt32LE();
        }
    }

    // Parse name if present
    if (m_flags & AdvertFlags::NameMask) {
        if (reader.hasRemaining()) {
            m_name = reader.readString();
        }
    }
}

} // namespace MeshCore
