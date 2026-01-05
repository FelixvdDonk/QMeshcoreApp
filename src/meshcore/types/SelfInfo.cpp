#include "SelfInfo.h"

namespace MeshCore {

SelfInfo::SelfInfo(AdvertType type, quint8 txPower, quint8 maxTxPower,
                   const QByteArray &publicKey, qint32 latitude, qint32 longitude,
                   bool manualAddContacts, quint32 radioFreq, quint32 radioBw,
                   quint8 radioSf, quint8 radioCr, const QString &name)
    : m_type(type)
    , m_txPower(txPower)
    , m_maxTxPower(maxTxPower)
    , m_publicKey(publicKey)
    , m_latitude(latitude)
    , m_longitude(longitude)
    , m_manualAddContacts(manualAddContacts)
    , m_radioFreq(radioFreq)
    , m_radioBw(radioBw)
    , m_radioSf(radioSf)
    , m_radioCr(radioCr)
    , m_name(name)
{
}

QString SelfInfo::publicKeyHex() const
{
    return QString::fromLatin1(m_publicKey.toHex());
}

double SelfInfo::latitudeDecimal() const
{
    return static_cast<double>(m_latitude) / 1e6;
}

double SelfInfo::longitudeDecimal() const
{
    return static_cast<double>(m_longitude) / 1e6;
}

QString SelfInfo::typeString() const
{
    switch (m_type) {
    case AdvertType::None: return QStringLiteral("None");
    case AdvertType::Chat: return QStringLiteral("Chat");
    case AdvertType::Repeater: return QStringLiteral("Repeater");
    case AdvertType::Room: return QStringLiteral("Room");
    default: return QStringLiteral("Unknown");
    }
}

double SelfInfo::radioFreqMhz() const
{
    // Frequency is stored in kHz, convert to MHz
    return static_cast<double>(m_radioFreq) / 1e3;
}

double SelfInfo::radioBwKhz() const
{
    // Bandwidth is stored in Hz, convert to kHz
    return static_cast<double>(m_radioBw) / 1e3;
}

} // namespace MeshCore
