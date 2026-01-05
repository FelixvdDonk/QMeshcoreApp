#include "ChannelModel.h"

namespace MeshCore {

ChannelModel::ChannelModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChannelModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_channels.size());
}

QVariant ChannelModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_channels.size()) {
        return {};
    }

    const ChannelInfo &channel = m_channels.at(index.row());

    switch (role) {
    case IndexRole:
        return channel.index();
    case NameRole:
        return channel.name();
    case SecretRole:
        return channel.secret();
    case SecretHexRole:
        return channel.secretHex();
    case IsEmptyRole:
        return channel.isEmpty();
    case ChannelInfoRole:
        return QVariant::fromValue(channel);
    default:
        return {};
    }
}

QHash<int, QByteArray> ChannelModel::roleNames() const
{
    static QHash<int, QByteArray> roles{
        {IndexRole, "channelIndex"},
        {NameRole, "name"},
        {SecretRole, "secret"},
        {SecretHexRole, "secretHex"},
        {IsEmptyRole, "isEmpty"},
        {ChannelInfoRole, "channelInfo"}
    };
    return roles;
}

ChannelInfo ChannelModel::get(int index) const
{
    if (index < 0 || index >= m_channels.size()) {
        return {};
    }
    return m_channels.at(index);
}

ChannelInfo ChannelModel::findByName(const QString &name) const
{
    for (const auto &channel : m_channels) {
        if (channel.name() == name) {
            return channel;
        }
    }
    return {};
}

void ChannelModel::clear()
{
    if (m_channels.isEmpty()) {
        return;
    }
    beginResetModel();
    m_channels.clear();
    endResetModel();
    Q_EMIT countChanged();
}

void ChannelModel::addChannel(const ChannelInfo &channel)
{
    beginInsertRows(QModelIndex(), static_cast<int>(m_channels.size()), static_cast<int>(m_channels.size()));
    m_channels.append(channel);
    endInsertRows();
    Q_EMIT countChanged();
}

void ChannelModel::setChannels(const QList<ChannelInfo> &channels)
{
    beginResetModel();
    m_channels = channels;
    endResetModel();
    Q_EMIT countChanged();
}

void ChannelModel::updateChannel(const ChannelInfo &channel)
{
    for (int i = 0; i < m_channels.size(); ++i) {
        if (m_channels.at(i).index() == channel.index()) {
            m_channels[i] = channel;
            QModelIndex idx = index(i);
            Q_EMIT dataChanged(idx, idx);
            return;
        }
    }
    addChannel(channel);
}

} // namespace MeshCore
