#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace MeshCore {

/**
 * @brief Contains firmware and device information
 */
class DeviceInfo
{
    Q_GADGET

    Q_PROPERTY(qint8 firmwareVersion READ firmwareVersion CONSTANT)
    Q_PROPERTY(QString firmwareBuildDate READ firmwareBuildDate CONSTANT)
    Q_PROPERTY(QString manufacturerModel READ manufacturerModel CONSTANT)

public:
    DeviceInfo() = default;
    DeviceInfo(qint8 firmwareVersion, const QString &firmwareBuildDate,
               const QString &manufacturerModel);

    [[nodiscard]] qint8 firmwareVersion() const { return m_firmwareVersion; }
    [[nodiscard]] QString firmwareBuildDate() const { return m_firmwareBuildDate; }
    [[nodiscard]] QString manufacturerModel() const { return m_manufacturerModel; }

    [[nodiscard]] bool isValid() const { return m_firmwareVersion > 0; }

private:
    qint8 m_firmwareVersion = 0;
    QString m_firmwareBuildDate;
    QString m_manufacturerModel;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::DeviceInfo)

#endif // DEVICEINFO_H
