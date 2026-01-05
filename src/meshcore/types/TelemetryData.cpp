#include "TelemetryData.h"
#include "../utils/CayenneLpp.h"

namespace MeshCore {

TelemetryValue::TelemetryValue(quint8 channel, quint8 type, const QVariant &value)
    : m_channel(channel)
    , m_type(type)
    , m_value(value)
{
}

QString TelemetryValue::typeName() const
{
    return CayenneLpp::typeName(m_type);
}

TelemetryData::TelemetryData(const QByteArray &senderPublicKeyPrefix, const QList<TelemetryValue> &values)
    : m_senderPublicKeyPrefix(senderPublicKeyPrefix)
    , m_values(values)
{
}

TelemetryData TelemetryData::fromLppData(const QByteArray &pubKeyPrefix, const QByteArray &lppData)
{
    QList<TelemetryValue> values = CayenneLpp::parse(lppData);
    return TelemetryData(pubKeyPrefix, values);
}

QString TelemetryData::senderPublicKeyPrefixHex() const
{
    return QString::fromLatin1(m_senderPublicKeyPrefix.toHex());
}

QVariantList TelemetryData::valuesAsVariant() const
{
    QVariantList list;
    for (const auto &v : m_values) {
        QVariantMap map;
        map[QStringLiteral("channel")] = v.channel();
        map[QStringLiteral("type")] = v.type();
        map[QStringLiteral("typeName")] = v.typeName();
        map[QStringLiteral("value")] = v.value();
        list.append(map);
    }
    return list;
}

} // namespace MeshCore
