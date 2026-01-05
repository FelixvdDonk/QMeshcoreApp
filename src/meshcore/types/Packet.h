#ifndef PACKET_H
#define PACKET_H

#include <QObject>
#include <QByteArray>
#include <QtQml/qqmlregistration.h>
#include "../MeshCoreConstants.h"

namespace MeshCore {

class Advert;

/**
 * @brief Represents a raw MeshCore packet
 */
class Packet
{
    Q_GADGET

    Q_PROPERTY(quint8 header READ header CONSTANT)
    Q_PROPERTY(QByteArray path READ path CONSTANT)
    Q_PROPERTY(QByteArray payload READ payload CONSTANT)
    Q_PROPERTY(RouteType routeType READ routeType CONSTANT)
    Q_PROPERTY(QString routeTypeString READ routeTypeString CONSTANT)
    Q_PROPERTY(PayloadType payloadType READ payloadType CONSTANT)
    Q_PROPERTY(QString payloadTypeString READ payloadTypeString CONSTANT)
    Q_PROPERTY(quint8 payloadVersion READ payloadVersion CONSTANT)
    Q_PROPERTY(bool isDoNotRetransmit READ isDoNotRetransmit CONSTANT)

public:
    Packet() = default;
    Packet(quint8 header, const QByteArray &path, const QByteArray &payload);

    static Packet fromBytes(const QByteArray &data);

    [[nodiscard]] quint8 header() const { return m_header; }
    [[nodiscard]] QByteArray path() const { return m_path; }
    [[nodiscard]] QByteArray payload() const { return m_payload; }

    [[nodiscard]] RouteType routeType() const;
    [[nodiscard]] QString routeTypeString() const;
    [[nodiscard]] PayloadType payloadType() const;
    [[nodiscard]] QString payloadTypeString() const;
    [[nodiscard]] quint8 payloadVersion() const;

    [[nodiscard]] bool isRouteFlood() const { return routeType() == RouteType::Flood; }
    [[nodiscard]] bool isRouteDirect() const { return routeType() == RouteType::Direct; }
    [[nodiscard]] bool isDoNotRetransmit() const { return m_header == 0xFF; }

    void markDoNotRetransmit() { m_header = 0xFF; }

    // Payload parsing helpers
    [[nodiscard]] Advert parseAdvertPayload() const;

    [[nodiscard]] bool isValid() const { return m_header != 0 || !m_payload.isEmpty(); }

private:
    quint8 m_header = 0;
    QByteArray m_path;
    QByteArray m_payload;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::Packet)

#endif // PACKET_H
