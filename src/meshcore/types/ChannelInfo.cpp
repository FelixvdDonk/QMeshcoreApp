#include "ChannelInfo.h"

namespace MeshCore {

ChannelInfo::ChannelInfo(quint8 index, const QString &name, const QByteArray &secret)
    : m_index(index)
    , m_name(name)
    , m_secret(secret)
{
}

QString ChannelInfo::secretHex() const
{
    return QString::fromLatin1(m_secret.toHex());
}

bool ChannelInfo::isEmpty() const
{
    // Channel is empty if name is empty and secret is all zeros
    if (!m_name.isEmpty()) {
        return false;
    }

    for (char c : m_secret) {
        if (c != 0) {
            return false;
        }
    }

    return true;
}

} // namespace MeshCore
