#ifndef CONTACTMESSAGE_H
#define CONTACTMESSAGE_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>
#include "../MeshCoreConstants.h"

namespace MeshCore {

/**
 * @brief Represents a direct message received from a contact
 */
class ContactMessage
{
    Q_GADGET

    Q_PROPERTY(QByteArray senderPublicKeyPrefix READ senderPublicKeyPrefix CONSTANT)
    Q_PROPERTY(QString senderPublicKeyPrefixHex READ senderPublicKeyPrefixHex CONSTANT)
    Q_PROPERTY(quint8 pathLen READ pathLen CONSTANT)
    Q_PROPERTY(TxtType textType READ textType CONSTANT)
    Q_PROPERTY(quint32 senderTimestamp READ senderTimestamp CONSTANT)
    Q_PROPERTY(QString text READ text CONSTANT)
    Q_PROPERTY(QDateTime dateTime READ dateTime CONSTANT)

public:
    ContactMessage() = default;
    ContactMessage(const QByteArray &senderPublicKeyPrefix, quint8 pathLen,
                   TxtType textType, quint32 senderTimestamp, const QString &text);

    [[nodiscard]] QByteArray senderPublicKeyPrefix() const { return m_senderPublicKeyPrefix; }
    [[nodiscard]] QString senderPublicKeyPrefixHex() const;
    [[nodiscard]] quint8 pathLen() const { return m_pathLen; }
    [[nodiscard]] TxtType textType() const { return m_textType; }
    [[nodiscard]] quint32 senderTimestamp() const { return m_senderTimestamp; }
    [[nodiscard]] QString text() const { return m_text; }
    [[nodiscard]] QDateTime dateTime() const;

private:
    QByteArray m_senderPublicKeyPrefix;  // 6 bytes
    quint8 m_pathLen = 0;
    TxtType m_textType = TxtType::Plain;
    quint32 m_senderTimestamp = 0;
    QString m_text;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::ContactMessage)

#endif // CONTACTMESSAGE_H
