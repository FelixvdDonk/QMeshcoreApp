#ifndef RXLOGENTRY_H
#define RXLOGENTRY_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>

namespace MeshCore {

/**
 * @brief Represents a received radio packet log entry
 * 
 * Parses MeshCore packet header to extract routing and payload information.
 * Header byte format:
 *   Bits 0-1 (0x03): Route Type
 *   Bits 2-5 (0x3C): Payload Type  
 *   Bits 6-7 (0xC0): Payload Version
 */
class RxLogEntry
{
    Q_GADGET

    Q_PROPERTY(QDateTime timestamp READ timestamp CONSTANT)
    Q_PROPERTY(double snr READ snr CONSTANT)
    Q_PROPERTY(int rssi READ rssi CONSTANT)
    Q_PROPERTY(QByteArray rawData READ rawData CONSTANT)
    Q_PROPERTY(QString rawDataHex READ rawDataHex CONSTANT)
    Q_PROPERTY(int dataLength READ dataLength CONSTANT)
    
    // Parsed packet info
    Q_PROPERTY(int routeType READ routeType CONSTANT)
    Q_PROPERTY(QString routeTypeName READ routeTypeName CONSTANT)
    Q_PROPERTY(int payloadType READ payloadType CONSTANT)
    Q_PROPERTY(QString payloadTypeName READ payloadTypeName CONSTANT)
    Q_PROPERTY(int payloadVersion READ payloadVersion CONSTANT)
    Q_PROPERTY(int hopCount READ hopCount CONSTANT)
    Q_PROPERTY(int pathLength READ pathLength CONSTANT)
    
    // Payload-specific parsed fields
    Q_PROPERTY(int destHash READ destHash CONSTANT)
    Q_PROPERTY(int srcHash READ srcHash CONSTANT)
    Q_PROPERTY(QString advertName READ advertName CONSTANT)
    Q_PROPERTY(int advertType READ advertType CONSTANT)
    Q_PROPERTY(QString advertTypeName READ advertTypeName CONSTANT)
    Q_PROPERTY(bool hasLocation READ hasLocation CONSTANT)
    Q_PROPERTY(double latitude READ latitude CONSTANT)
    Q_PROPERTY(double longitude READ longitude CONSTANT)

public:
    // Route types
    enum RouteType {
        RouteTransportFlood = 0x00,
        RouteFlood = 0x01,
        RouteDirect = 0x02,
        RouteTransportDirect = 0x03
    };
    Q_ENUM(RouteType)

    // Payload types
    enum PayloadType {
        PayloadRequest = 0x00,
        PayloadResponse = 0x01,
        PayloadTextMsg = 0x02,
        PayloadAck = 0x03,
        PayloadAdvert = 0x04,
        PayloadGroupText = 0x05,
        PayloadGroupData = 0x06,
        PayloadAnonRequest = 0x07,
        PayloadPath = 0x08,
        PayloadTrace = 0x09,
        PayloadMultipart = 0x0A,
        PayloadControl = 0x0B,
        PayloadRawCustom = 0x0F
    };
    Q_ENUM(PayloadType)

    RxLogEntry() = default;
    RxLogEntry(double snr, qint8 rssi, const QByteArray &rawData);

    [[nodiscard]] QDateTime timestamp() const { return m_timestamp; }
    [[nodiscard]] double snr() const { return m_snr; }
    [[nodiscard]] int rssi() const { return m_rssi; }
    [[nodiscard]] QByteArray rawData() const { return m_rawData; }
    [[nodiscard]] QString rawDataHex() const;
    [[nodiscard]] int dataLength() const { return m_rawData.size(); }
    
    // Parsed packet info
    [[nodiscard]] int routeType() const { return m_routeType; }
    [[nodiscard]] QString routeTypeName() const;
    [[nodiscard]] int payloadType() const { return m_payloadType; }
    [[nodiscard]] QString payloadTypeName() const;
    [[nodiscard]] int payloadVersion() const { return m_payloadVersion; }
    [[nodiscard]] int hopCount() const { return m_hopCount; }
    [[nodiscard]] int pathLength() const { return m_pathLength; }
    
    // Payload-specific parsed fields
    [[nodiscard]] int destHash() const { return m_destHash; }
    [[nodiscard]] int srcHash() const { return m_srcHash; }
    [[nodiscard]] QString advertName() const { return m_advertName; }
    [[nodiscard]] int advertType() const { return m_advertType; }
    [[nodiscard]] QString advertTypeName() const;
    [[nodiscard]] bool hasLocation() const { return m_hasLocation; }
    [[nodiscard]] double latitude() const { return m_latitude; }
    [[nodiscard]] double longitude() const { return m_longitude; }

private:
    void parsePacket();
    void parsePayload(const QByteArray &payload);
    void parseAdvert(const QByteArray &payload);

    QDateTime m_timestamp;
    double m_snr = 0.0;
    qint8 m_rssi = 0;
    QByteArray m_rawData;
    
    // Parsed header fields
    int m_routeType = -1;
    int m_payloadType = -1;
    int m_payloadVersion = 0;
    int m_hopCount = 0;
    int m_pathLength = 0;
    
    // Payload-specific fields
    int m_destHash = -1;
    int m_srcHash = -1;
    QString m_advertName;
    int m_advertType = 0;
    bool m_hasLocation = false;
    double m_latitude = 0.0;
    double m_longitude = 0.0;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::RxLogEntry)

#endif // RXLOGENTRY_H
