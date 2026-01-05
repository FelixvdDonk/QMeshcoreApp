#include "ChannelMessage.h"
#include <QTimeZone>

namespace MeshCore {

ChannelMessage::ChannelMessage(qint8 channelIndex, quint8 pathLen, TxtType textType,
                               quint32 senderTimestamp, const QString &text)
    : m_channelIndex(channelIndex)
    , m_pathLen(pathLen)
    , m_textType(textType)
    , m_senderTimestamp(senderTimestamp)
    , m_text(text)
{
}

QDateTime ChannelMessage::dateTime() const
{
    return QDateTime::fromSecsSinceEpoch(m_senderTimestamp, QTimeZone::UTC);
}

} // namespace MeshCore
