#ifndef CONTACT_H
#define CONTACT_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include "../MeshCoreConstants.h"

namespace MeshCore {

/**
 * @brief Represents a contact stored on the MeshCore device
 *
 * This class is a QML-friendly gadget type that holds contact information
 * received from the device.
 */
class Contact
{
    Q_GADGET

    Q_PROPERTY(QByteArray publicKey READ publicKey CONSTANT)
    Q_PROPERTY(QString publicKeyHex READ publicKeyHex CONSTANT)
    Q_PROPERTY(AdvertType type READ type CONSTANT)
    Q_PROPERTY(quint8 flags READ flags CONSTANT)
    Q_PROPERTY(qint8 outPathLen READ outPathLen CONSTANT)
    Q_PROPERTY(QByteArray outPath READ outPath CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(quint32 lastAdvert READ lastAdvert CONSTANT)
    Q_PROPERTY(qint32 latitude READ latitude CONSTANT)
    Q_PROPERTY(qint32 longitude READ longitude CONSTANT)
    Q_PROPERTY(quint32 lastModified READ lastModified CONSTANT)
    Q_PROPERTY(double latitudeDecimal READ latitudeDecimal CONSTANT)
    Q_PROPERTY(double longitudeDecimal READ longitudeDecimal CONSTANT)
    Q_PROPERTY(QString typeString READ typeString CONSTANT)

public:
    Contact() = default;
    Contact(const QByteArray &publicKey, AdvertType type, quint8 flags,
            qint8 outPathLen, const QByteArray &outPath, const QString &name,
            quint32 lastAdvert, qint32 latitude, qint32 longitude, quint32 lastModified);

    [[nodiscard]] QByteArray publicKey() const { return m_publicKey; }
    [[nodiscard]] QString publicKeyHex() const;
    [[nodiscard]] AdvertType type() const { return m_type; }
    [[nodiscard]] quint8 flags() const { return m_flags; }
    [[nodiscard]] qint8 outPathLen() const { return m_outPathLen; }
    [[nodiscard]] QByteArray outPath() const { return m_outPath; }
    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] quint32 lastAdvert() const { return m_lastAdvert; }
    [[nodiscard]] qint32 latitude() const { return m_latitude; }
    [[nodiscard]] qint32 longitude() const { return m_longitude; }
    [[nodiscard]] quint32 lastModified() const { return m_lastModified; }

    // Convenience methods
    [[nodiscard]] double latitudeDecimal() const;
    [[nodiscard]] double longitudeDecimal() const;
    [[nodiscard]] QString typeString() const;
    [[nodiscard]] QByteArray publicKeyPrefix(int length = 6) const;

    // Comparison
    bool operator==(const Contact &other) const;
    bool operator!=(const Contact &other) const { return !(*this == other); }

private:
    QByteArray m_publicKey;    // 32 bytes
    AdvertType m_type = AdvertType::None;
    quint8 m_flags = 0;
    qint8 m_outPathLen = 0;
    QByteArray m_outPath;      // 64 bytes max
    QString m_name;            // 32 chars max
    quint32 m_lastAdvert = 0;
    qint32 m_latitude = 0;
    qint32 m_longitude = 0;
    quint32 m_lastModified = 0;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::Contact)

#endif // CONTACT_H
