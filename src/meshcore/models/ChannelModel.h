#ifndef CHANNELMODEL_H
#define CHANNELMODEL_H

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include "../types/ChannelInfo.h"

namespace MeshCore {

/**
 * @brief List model for channels, suitable for QML ListView
 */
class ChannelModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IndexRole = Qt::UserRole + 1,
        NameRole,
        SecretRole,
        SecretHexRole,
        IsEmptyRole,
        ChannelInfoRole
    };

    explicit ChannelModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_channels.size()); }

    Q_INVOKABLE MeshCore::ChannelInfo get(int index) const;
    Q_INVOKABLE MeshCore::ChannelInfo findByName(const QString &name) const;

    void clear();
    void addChannel(const ChannelInfo &channel);
    void setChannels(const QList<ChannelInfo> &channels);
    void updateChannel(const ChannelInfo &channel);

Q_SIGNALS:
    void countChanged();

private:
    QList<ChannelInfo> m_channels;
};

} // namespace MeshCore

#endif // CHANNELMODEL_H
