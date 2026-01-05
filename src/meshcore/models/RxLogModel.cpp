#include "RxLogModel.h"

namespace MeshCore {

RxLogModel::RxLogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RxLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.count();
}

QVariant RxLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.count())
        return QVariant();

    const RxLogEntry &entry = m_entries.at(index.row());

    switch (role) {
    case TimestampRole:
        return entry.timestamp();
    case TimestampStringRole:
        return entry.timestamp().toString("hh:mm:ss.zzz");
    case SnrRole:
        return entry.snr();
    case RssiRole:
        return entry.rssi();
    case RawDataRole:
        return entry.rawData();
    case RawDataHexRole:
        return entry.rawDataHex();
    case DataLengthRole:
        return entry.dataLength();
    case RouteTypeRole:
        return entry.routeType();
    case RouteTypeNameRole:
        return entry.routeTypeName();
    case PayloadTypeRole:
        return entry.payloadType();
    case PayloadTypeNameRole:
        return entry.payloadTypeName();
    case PayloadVersionRole:
        return entry.payloadVersion();
    case HopCountRole:
        return entry.hopCount();
    case PathLengthRole:
        return entry.pathLength();
    case DestHashRole:
        return entry.destHash();
    case SrcHashRole:
        return entry.srcHash();
    case AdvertNameRole:
        return entry.advertName();
    case AdvertTypeRole:
        return entry.advertType();
    case AdvertTypeNameRole:
        return entry.advertTypeName();
    case HasLocationRole:
        return entry.hasLocation();
    case LatitudeRole:
        return entry.latitude();
    case LongitudeRole:
        return entry.longitude();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> RxLogModel::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {TimestampStringRole, "timestampString"},
        {SnrRole, "snr"},
        {RssiRole, "rssi"},
        {RawDataRole, "rawData"},
        {RawDataHexRole, "rawDataHex"},
        {DataLengthRole, "dataLength"},
        {RouteTypeRole, "routeType"},
        {RouteTypeNameRole, "routeTypeName"},
        {PayloadTypeRole, "payloadType"},
        {PayloadTypeNameRole, "payloadTypeName"},
        {PayloadVersionRole, "payloadVersion"},
        {HopCountRole, "hopCount"},
        {PathLengthRole, "pathLength"},
        {DestHashRole, "destHash"},
        {SrcHashRole, "srcHash"},
        {AdvertNameRole, "advertName"},
        {AdvertTypeRole, "advertType"},
        {AdvertTypeNameRole, "advertTypeName"},
        {HasLocationRole, "hasLocation"},
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"}
    };
}

void RxLogModel::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        Q_EMIT enabledChanged();
    }
}

void RxLogModel::setMaxEntries(int max)
{
    if (m_maxEntries != max) {
        m_maxEntries = max;
        Q_EMIT maxEntriesChanged();
        
        // Trim if needed
        if (m_entries.count() > m_maxEntries) {
            int toRemove = m_entries.count() - m_maxEntries;
            beginRemoveRows(QModelIndex(), 0, toRemove - 1);
            m_entries.remove(0, toRemove);
            endRemoveRows();
            Q_EMIT countChanged();
        }
    }
}

void RxLogModel::addEntry(double snr, qint8 rssi, const QByteArray &rawData)
{
    if (!m_enabled)
        return;

    // Remove oldest entries if we're at capacity
    if (m_entries.count() >= m_maxEntries) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_entries.removeFirst();
        endRemoveRows();
    }

    // Add new entry at the end (newest at bottom)
    beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count());
    m_entries.append(RxLogEntry(snr, rssi, rawData));
    endInsertRows();
    
    Q_EMIT countChanged();
}

void RxLogModel::clear()
{
    if (m_entries.isEmpty())
        return;

    beginResetModel();
    m_entries.clear();
    endResetModel();
    Q_EMIT countChanged();
}

} // namespace MeshCore
