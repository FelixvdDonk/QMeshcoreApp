#ifndef TELEMETRYDATA_H
#define TELEMETRYDATA_H

#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace MeshCore {

/**
 * @brief Single telemetry value in CayenneLPP format
 */
class TelemetryValue
{
    Q_GADGET

    Q_PROPERTY(quint8 channel READ channel CONSTANT)
    Q_PROPERTY(quint8 type READ type CONSTANT)
    Q_PROPERTY(QString typeName READ typeName CONSTANT)
    Q_PROPERTY(QVariant value READ value CONSTANT)

public:
    TelemetryValue() = default;
    TelemetryValue(quint8 channel, quint8 type, const QVariant &value);

    [[nodiscard]] quint8 channel() const { return m_channel; }
    [[nodiscard]] quint8 type() const { return m_type; }
    [[nodiscard]] QString typeName() const;
    [[nodiscard]] QVariant value() const { return m_value; }

private:
    quint8 m_channel = 0;
    quint8 m_type = 0;
    QVariant m_value;
};

/**
 * @brief Telemetry data from a sensor node (parsed CayenneLPP)
 */
class TelemetryData
{
    Q_GADGET

    Q_PROPERTY(QByteArray senderPublicKeyPrefix READ senderPublicKeyPrefix CONSTANT)
    Q_PROPERTY(QString senderPublicKeyPrefixHex READ senderPublicKeyPrefixHex CONSTANT)
    Q_PROPERTY(QVariantList values READ valuesAsVariant CONSTANT)

public:
    TelemetryData() = default;
    TelemetryData(const QByteArray &senderPublicKeyPrefix, const QList<TelemetryValue> &values);

    static TelemetryData fromLppData(const QByteArray &pubKeyPrefix, const QByteArray &lppData);

    [[nodiscard]] QByteArray senderPublicKeyPrefix() const { return m_senderPublicKeyPrefix; }
    [[nodiscard]] QString senderPublicKeyPrefixHex() const;
    [[nodiscard]] QList<TelemetryValue> values() const { return m_values; }
    [[nodiscard]] QVariantList valuesAsVariant() const;

private:
    QByteArray m_senderPublicKeyPrefix;  // 6 bytes
    QList<TelemetryValue> m_values;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::TelemetryValue)
Q_DECLARE_METATYPE(MeshCore::TelemetryData)

#endif // TELEMETRYDATA_H
