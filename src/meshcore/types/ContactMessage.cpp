#include "ContactMessage.h"
#include <QTimeZone>

namespace MeshCore {

ContactMessage::ContactMessage(const QByteArray &senderPublicKeyPrefix, quint8 pathLen,
                               TxtType textType, quint32 senderTimestamp, const QString &text)
    : m_senderPublicKeyPrefix(senderPublicKeyPrefix)
    , m_pathLen(pathLen)
    , m_textType(textType)
    , m_senderTimestamp(senderTimestamp)
    , m_text(text)
{
}

QString ContactMessage::senderPublicKeyPrefixHex() const
{
    return QString::fromLatin1(m_senderPublicKeyPrefix.toHex());
}

QDateTime ContactMessage::dateTime() const
{
    return QDateTime::fromSecsSinceEpoch(m_senderTimestamp, QTimeZone::UTC);
}

} // namespace MeshCore
