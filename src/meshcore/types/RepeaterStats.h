#ifndef REPEATERSTATS_H
#define REPEATERSTATS_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace MeshCore {

/**
 * @brief Statistics from a repeater node
 */
class RepeaterStats
{
    Q_GADGET

    Q_PROPERTY(quint16 batteryMilliVolts READ batteryMilliVolts CONSTANT)
    Q_PROPERTY(quint16 currentTxQueueLength READ currentTxQueueLength CONSTANT)
    Q_PROPERTY(qint16 noiseFloor READ noiseFloor CONSTANT)
    Q_PROPERTY(qint16 lastRssi READ lastRssi CONSTANT)
    Q_PROPERTY(quint32 packetsReceived READ packetsReceived CONSTANT)
    Q_PROPERTY(quint32 packetsSent READ packetsSent CONSTANT)
    Q_PROPERTY(quint32 totalAirTimeSecs READ totalAirTimeSecs CONSTANT)
    Q_PROPERTY(quint32 totalUpTimeSecs READ totalUpTimeSecs CONSTANT)
    Q_PROPERTY(quint32 sentFlood READ sentFlood CONSTANT)
    Q_PROPERTY(quint32 sentDirect READ sentDirect CONSTANT)
    Q_PROPERTY(quint32 recvFlood READ recvFlood CONSTANT)
    Q_PROPERTY(quint32 recvDirect READ recvDirect CONSTANT)
    Q_PROPERTY(quint16 errorEvents READ errorEvents CONSTANT)
    Q_PROPERTY(qint16 lastSnr READ lastSnr CONSTANT)
    Q_PROPERTY(quint16 directDuplicates READ directDuplicates CONSTANT)
    Q_PROPERTY(quint16 floodDuplicates READ floodDuplicates CONSTANT)
    Q_PROPERTY(double batteryVolts READ batteryVolts CONSTANT)
    Q_PROPERTY(double lastSnrDecimal READ lastSnrDecimal CONSTANT)

public:
    RepeaterStats() = default;
    RepeaterStats(quint16 batteryMilliVolts, quint16 currentTxQueueLength,
                  qint16 noiseFloor, qint16 lastRssi, quint32 packetsReceived,
                  quint32 packetsSent, quint32 totalAirTimeSecs, quint32 totalUpTimeSecs,
                  quint32 sentFlood, quint32 sentDirect, quint32 recvFlood, quint32 recvDirect,
                  quint16 errorEvents, qint16 lastSnr, quint16 directDuplicates, quint16 floodDuplicates);

    [[nodiscard]] quint16 batteryMilliVolts() const { return m_batteryMilliVolts; }
    [[nodiscard]] quint16 currentTxQueueLength() const { return m_currentTxQueueLength; }
    [[nodiscard]] qint16 noiseFloor() const { return m_noiseFloor; }
    [[nodiscard]] qint16 lastRssi() const { return m_lastRssi; }
    [[nodiscard]] quint32 packetsReceived() const { return m_packetsReceived; }
    [[nodiscard]] quint32 packetsSent() const { return m_packetsSent; }
    [[nodiscard]] quint32 totalAirTimeSecs() const { return m_totalAirTimeSecs; }
    [[nodiscard]] quint32 totalUpTimeSecs() const { return m_totalUpTimeSecs; }
    [[nodiscard]] quint32 sentFlood() const { return m_sentFlood; }
    [[nodiscard]] quint32 sentDirect() const { return m_sentDirect; }
    [[nodiscard]] quint32 recvFlood() const { return m_recvFlood; }
    [[nodiscard]] quint32 recvDirect() const { return m_recvDirect; }
    [[nodiscard]] quint16 errorEvents() const { return m_errorEvents; }
    [[nodiscard]] qint16 lastSnr() const { return m_lastSnr; }
    [[nodiscard]] quint16 directDuplicates() const { return m_directDuplicates; }
    [[nodiscard]] quint16 floodDuplicates() const { return m_floodDuplicates; }

    // Convenience
    [[nodiscard]] double batteryVolts() const { return m_batteryMilliVolts / 1000.0; }
    [[nodiscard]] double lastSnrDecimal() const { return m_lastSnr / 4.0; }

private:
    quint16 m_batteryMilliVolts = 0;
    quint16 m_currentTxQueueLength = 0;
    qint16 m_noiseFloor = 0;
    qint16 m_lastRssi = 0;
    quint32 m_packetsReceived = 0;
    quint32 m_packetsSent = 0;
    quint32 m_totalAirTimeSecs = 0;
    quint32 m_totalUpTimeSecs = 0;
    quint32 m_sentFlood = 0;
    quint32 m_sentDirect = 0;
    quint32 m_recvFlood = 0;
    quint32 m_recvDirect = 0;
    quint16 m_errorEvents = 0;
    qint16 m_lastSnr = 0;
    quint16 m_directDuplicates = 0;
    quint16 m_floodDuplicates = 0;
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::RepeaterStats)

#endif // REPEATERSTATS_H
