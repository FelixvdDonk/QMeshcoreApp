#ifndef TRACEDATA_H
#define TRACEDATA_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QtQml/qqmlregistration.h>

namespace MeshCore {

/**
 * @brief Path trace response data
 */
class TraceData
{
    Q_GADGET

    Q_PROPERTY(quint8 pathLen READ pathLen CONSTANT)
    Q_PROPERTY(quint8 flags READ flags CONSTANT)
    Q_PROPERTY(quint32 tag READ tag CONSTANT)
    Q_PROPERTY(quint32 authCode READ authCode CONSTANT)
    Q_PROPERTY(QByteArray pathHashes READ pathHashes CONSTANT)
    Q_PROPERTY(QByteArray pathSnrs READ pathSnrs CONSTANT)
    Q_PROPERTY(double lastSnr READ lastSnr CONSTANT)

public:
    TraceData() = default;
    TraceData(quint8 pathLen, quint8 flags, quint32 tag, quint32 authCode,
              const QByteArray &pathHashes, const QByteArray &pathSnrs, qint8 lastSnrRaw);

    [[nodiscard]] quint8 pathLen() const { return m_pathLen; }
    [[nodiscard]] quint8 flags() const { return m_flags; }
    [[nodiscard]] quint32 tag() const { return m_tag; }
    [[nodiscard]] quint32 authCode() const { return m_authCode; }
    [[nodiscard]] QByteArray pathHashes() const { return m_pathHashes; }
    [[nodiscard]] QByteArray pathSnrs() const { return m_pathSnrs; }
    [[nodiscard]] double lastSnr() const { return m_lastSnrRaw / 4.0; }

    // Get SNR values as a list of doubles
    Q_INVOKABLE QList<double> snrValues() const;

private:
    quint8 m_pathLen = 0;
    quint8 m_flags = 0;
    quint32 m_tag = 0;
    quint32 m_authCode = 0;
    QByteArray m_pathHashes;
    QByteArray m_pathSnrs;
    qint8 m_lastSnrRaw = 0;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::TraceData)

#endif // TRACEDATA_H
