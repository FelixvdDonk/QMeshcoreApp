#include "DeviceInfo.h"

namespace MeshCore {

DeviceInfo::DeviceInfo(qint8 firmwareVersion, const QString &firmwareBuildDate,
                       const QString &manufacturerModel)
    : m_firmwareVersion(firmwareVersion)
    , m_firmwareBuildDate(firmwareBuildDate)
    , m_manufacturerModel(manufacturerModel)
{
}

} // namespace MeshCore
