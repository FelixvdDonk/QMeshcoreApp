#include "Contact.h"

namespace MeshCore {

Contact::Contact(const QByteArray &publicKey, AdvertType type, quint8 flags,
                 qint8 outPathLen, const QByteArray &outPath, const QString &name,
                 quint32 lastAdvert, qint32 latitude, qint32 longitude, quint32 lastModified)
    : m_publicKey(publicKey)
    , m_type(type)
    , m_flags(flags)
    , m_outPathLen(outPathLen)
    , m_outPath(outPath)
    , m_name(name)
    , m_lastAdvert(lastAdvert)
    , m_latitude(latitude)
    , m_longitude(longitude)
    , m_lastModified(lastModified)
{
}

QString Contact::publicKeyHex() const
{
    return QString::fromLatin1(m_publicKey.toHex());
}

double Contact::latitudeDecimal() const
{
    // Convert from device format (scaled integer, 1E6) to decimal degrees
    return static_cast<double>(m_latitude) / 1e6;
}

double Contact::longitudeDecimal() const
{
    return static_cast<double>(m_longitude) / 1e6;
}

QString Contact::typeString() const
{
    switch (m_type) {
    case AdvertType::None: return QStringLiteral("None");
    case AdvertType::Chat: return QStringLiteral("Chat");
    case AdvertType::Repeater: return QStringLiteral("Repeater");
    case AdvertType::Room: return QStringLiteral("Room");
    default: return QStringLiteral("Unknown");
    }
}

QByteArray Contact::publicKeyPrefix(int length) const
{
    return m_publicKey.left(length);
}

bool Contact::operator==(const Contact &other) const
{
    return m_publicKey == other.m_publicKey;
}

} // namespace MeshCore
