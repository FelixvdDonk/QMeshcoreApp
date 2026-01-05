#include "RepeaterStats.h"

namespace MeshCore {

RepeaterStats::RepeaterStats(quint16 batteryMilliVolts, quint16 currentTxQueueLength,
                             qint16 noiseFloor, qint16 lastRssi, quint32 packetsReceived,
                             quint32 packetsSent, quint32 totalAirTimeSecs, quint32 totalUpTimeSecs,
                             quint32 sentFlood, quint32 sentDirect, quint32 recvFlood, quint32 recvDirect,
                             quint16 errorEvents, qint16 lastSnr, quint16 directDuplicates, quint16 floodDuplicates)
    : m_batteryMilliVolts(batteryMilliVolts)
    , m_currentTxQueueLength(currentTxQueueLength)
    , m_noiseFloor(noiseFloor)
    , m_lastRssi(lastRssi)
    , m_packetsReceived(packetsReceived)
    , m_packetsSent(packetsSent)
    , m_totalAirTimeSecs(totalAirTimeSecs)
    , m_totalUpTimeSecs(totalUpTimeSecs)
    , m_sentFlood(sentFlood)
    , m_sentDirect(sentDirect)
    , m_recvFlood(recvFlood)
    , m_recvDirect(recvDirect)
    , m_errorEvents(errorEvents)
    , m_lastSnr(lastSnr)
    , m_directDuplicates(directDuplicates)
    , m_floodDuplicates(floodDuplicates)
{
}

} // namespace MeshCore
