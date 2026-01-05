#include "TraceData.h"

namespace MeshCore {

TraceData::TraceData(quint8 pathLen, quint8 flags, quint32 tag, quint32 authCode,
                     const QByteArray &pathHashes, const QByteArray &pathSnrs, qint8 lastSnrRaw)
    : m_pathLen(pathLen)
    , m_flags(flags)
    , m_tag(tag)
    , m_authCode(authCode)
    , m_pathHashes(pathHashes)
    , m_pathSnrs(pathSnrs)
    , m_lastSnrRaw(lastSnrRaw)
{
}

QList<double> TraceData::snrValues() const
{
    QList<double> values;
    values.reserve(m_pathSnrs.size());
    for (char c : m_pathSnrs) {
        values.append(static_cast<qint8>(c) / 4.0);
    }
    return values;
}

} // namespace MeshCore
