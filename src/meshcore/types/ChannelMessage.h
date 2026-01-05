#ifndef CHANNELMESSAGE_H
#define CHANNELMESSAGE_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>
#include "../MeshCoreConstants.h"

namespace MeshCore {

/**
 * @brief Represents a channel/group message
 */
class ChannelMessage
{
    Q_GADGET

    Q_PROPERTY(qint8 channelIndex READ channelIndex CONSTANT)
    Q_PROPERTY(quint8 pathLen READ pathLen CONSTANT)
    Q_PROPERTY(TxtType textType READ textType CONSTANT)
    Q_PROPERTY(quint32 senderTimestamp READ senderTimestamp CONSTANT)
    Q_PROPERTY(QString text READ text CONSTANT)
    Q_PROPERTY(QDateTime dateTime READ dateTime CONSTANT)
    Q_PROPERTY(bool isDirect READ isDirect CONSTANT)

public:
    ChannelMessage() = default;
    ChannelMessage(qint8 channelIndex, quint8 pathLen, TxtType textType,
                   quint32 senderTimestamp, const QString &text);

    [[nodiscard]] qint8 channelIndex() const { return m_channelIndex; }
    [[nodiscard]] quint8 pathLen() const { return m_pathLen; }
    [[nodiscard]] TxtType textType() const { return m_textType; }
    [[nodiscard]] quint32 senderTimestamp() const { return m_senderTimestamp; }
    [[nodiscard]] QString text() const { return m_text; }
    [[nodiscard]] QDateTime dateTime() const;
    [[nodiscard]] bool isDirect() const { return m_pathLen == 0xFF; }

private:
    qint8 m_channelIndex = 0;
    quint8 m_pathLen = 0;  // 0xFF means direct, otherwise hop count
    TxtType m_textType = TxtType::Plain;
    quint32 m_senderTimestamp = 0;
    QString m_text;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::ChannelMessage)

#endif // CHANNELMESSAGE_H
