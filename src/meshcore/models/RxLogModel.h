#ifndef RXLOGMODEL_H
#define RXLOGMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "../types/RxLogEntry.h"

namespace MeshCore {

/**
 * @brief Model for displaying RX log entries in a ListView
 */
class RxLogModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int maxEntries READ maxEntries WRITE setMaxEntries NOTIFY maxEntriesChanged)

public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        TimestampStringRole,
        SnrRole,
        RssiRole,
        RawDataRole,
        RawDataHexRole,
        DataLengthRole,
        // Parsed packet info
        RouteTypeRole,
        RouteTypeNameRole,
        PayloadTypeRole,
        PayloadTypeNameRole,
        PayloadVersionRole,
        HopCountRole,
        PathLengthRole,
        // Payload-specific
        DestHashRole,
        SrcHashRole,
        AdvertNameRole,
        AdvertTypeRole,
        AdvertTypeNameRole,
        HasLocationRole,
        LatitudeRole,
        LongitudeRole
    };

    explicit RxLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    int maxEntries() const { return m_maxEntries; }
    void setMaxEntries(int max);

public Q_SLOTS:
    void addEntry(double snr, qint8 rssi, const QByteArray &rawData);
    void clear();

Q_SIGNALS:
    void countChanged();
    void enabledChanged();
    void maxEntriesChanged();

private:
    QList<RxLogEntry> m_entries;
    bool m_enabled = false;
    int m_maxEntries = 500;  // Keep last 500 entries by default
};

} // namespace MeshCore

#endif // RXLOGMODEL_H
