#ifndef SELFINFO_H
#define SELFINFO_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include "../MeshCoreConstants.h"

namespace MeshCore {

/**
 * @brief Represents the device's own identity and configuration
 *
 * This is returned after AppStart command and contains info about
 * the connected MeshCore device.
 */
class SelfInfo
{
    Q_GADGET

    Q_PROPERTY(AdvertType type READ type CONSTANT)
    Q_PROPERTY(quint8 txPower READ txPower CONSTANT)
    Q_PROPERTY(quint8 maxTxPower READ maxTxPower CONSTANT)
    Q_PROPERTY(QByteArray publicKey READ publicKey CONSTANT)
    Q_PROPERTY(QString publicKeyHex READ publicKeyHex CONSTANT)
    Q_PROPERTY(qint32 latitude READ latitude CONSTANT)
    Q_PROPERTY(qint32 longitude READ longitude CONSTANT)
    Q_PROPERTY(bool manualAddContacts READ manualAddContacts CONSTANT)
    Q_PROPERTY(quint32 radioFreq READ radioFreq CONSTANT)
    Q_PROPERTY(quint32 radioBw READ radioBw CONSTANT)
    Q_PROPERTY(quint8 radioSf READ radioSf CONSTANT)
    Q_PROPERTY(quint8 radioCr READ radioCr CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(double latitudeDecimal READ latitudeDecimal CONSTANT)
    Q_PROPERTY(double longitudeDecimal READ longitudeDecimal CONSTANT)
    Q_PROPERTY(QString typeString READ typeString CONSTANT)
    Q_PROPERTY(double radioFreqMhz READ radioFreqMhz CONSTANT)
    Q_PROPERTY(double radioBwKhz READ radioBwKhz CONSTANT)

public:
    SelfInfo() = default;
    SelfInfo(AdvertType type, quint8 txPower, quint8 maxTxPower,
             const QByteArray &publicKey, qint32 latitude, qint32 longitude,
             bool manualAddContacts, quint32 radioFreq, quint32 radioBw,
             quint8 radioSf, quint8 radioCr, const QString &name);

    [[nodiscard]] AdvertType type() const { return m_type; }
    [[nodiscard]] quint8 txPower() const { return m_txPower; }
    [[nodiscard]] quint8 maxTxPower() const { return m_maxTxPower; }
    [[nodiscard]] QByteArray publicKey() const { return m_publicKey; }
    [[nodiscard]] QString publicKeyHex() const;
    [[nodiscard]] qint32 latitude() const { return m_latitude; }
    [[nodiscard]] qint32 longitude() const { return m_longitude; }
    [[nodiscard]] bool manualAddContacts() const { return m_manualAddContacts; }
    [[nodiscard]] quint32 radioFreq() const { return m_radioFreq; }
    [[nodiscard]] quint32 radioBw() const { return m_radioBw; }
    [[nodiscard]] quint8 radioSf() const { return m_radioSf; }
    [[nodiscard]] quint8 radioCr() const { return m_radioCr; }
    [[nodiscard]] QString name() const { return m_name; }

    // Convenience
    [[nodiscard]] double latitudeDecimal() const;
    [[nodiscard]] double longitudeDecimal() const;
    [[nodiscard]] QString typeString() const;
    [[nodiscard]] double radioFreqMhz() const;
    [[nodiscard]] double radioBwKhz() const;

    [[nodiscard]] bool isValid() const { return !m_publicKey.isEmpty(); }

private:
    AdvertType m_type = AdvertType::None;
    quint8 m_txPower = 0;
    quint8 m_maxTxPower = 0;
    QByteArray m_publicKey;  // 32 bytes
    qint32 m_latitude = 0;
    qint32 m_longitude = 0;
    bool m_manualAddContacts = false;
    quint32 m_radioFreq = 0;
    quint32 m_radioBw = 0;
    quint8 m_radioSf = 0;
    quint8 m_radioCr = 0;
    QString m_name;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::SelfInfo)

#endif // SELFINFO_H
