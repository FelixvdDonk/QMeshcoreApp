#ifndef ADVERT_H
#define ADVERT_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include "../MeshCoreConstants.h"

namespace MeshCore {

/**
 * @brief Represents an advertisement packet from a node
 */
class Advert
{
    Q_GADGET

    Q_PROPERTY(QByteArray publicKey READ publicKey CONSTANT)
    Q_PROPERTY(QString publicKeyHex READ publicKeyHex CONSTANT)
    Q_PROPERTY(quint32 timestamp READ timestamp CONSTANT)
    Q_PROPERTY(QByteArray signature READ signature CONSTANT)
    Q_PROPERTY(QByteArray appData READ appData CONSTANT)
    Q_PROPERTY(AdvertType type READ type CONSTANT)
    Q_PROPERTY(QString typeString READ typeString CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(bool hasLatLon READ hasLatLon CONSTANT)
    Q_PROPERTY(qint32 latitude READ latitude CONSTANT)
    Q_PROPERTY(qint32 longitude READ longitude CONSTANT)
    Q_PROPERTY(double latitudeDecimal READ latitudeDecimal CONSTANT)
    Q_PROPERTY(double longitudeDecimal READ longitudeDecimal CONSTANT)

public:
    Advert() = default;
    Advert(const QByteArray &publicKey, quint32 timestamp,
           const QByteArray &signature, const QByteArray &appData);

    static Advert fromBytes(const QByteArray &data);

    [[nodiscard]] QByteArray publicKey() const { return m_publicKey; }
    [[nodiscard]] QString publicKeyHex() const;
    [[nodiscard]] quint32 timestamp() const { return m_timestamp; }
    [[nodiscard]] QByteArray signature() const { return m_signature; }
    [[nodiscard]] QByteArray appData() const { return m_appData; }

    // Parsed app data
    [[nodiscard]] quint8 flags() const;
    [[nodiscard]] AdvertType type() const;
    [[nodiscard]] QString typeString() const;
    [[nodiscard]] bool hasLatLon() const;
    [[nodiscard]] bool hasName() const;
    [[nodiscard]] qint32 latitude() const { return m_latitude; }
    [[nodiscard]] qint32 longitude() const { return m_longitude; }
    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] double latitudeDecimal() const;
    [[nodiscard]] double longitudeDecimal() const;

    [[nodiscard]] bool isValid() const { return !m_publicKey.isEmpty(); }

private:
    void parseAppData();

    QByteArray m_publicKey;   // 32 bytes
    quint32 m_timestamp = 0;
    QByteArray m_signature;   // 64 bytes
    QByteArray m_appData;

    // Parsed values
    quint8 m_flags = 0;
    qint32 m_latitude = 0;
    qint32 m_longitude = 0;
    QString m_name;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::Advert)

#endif // ADVERT_H
