#include "MessageModel.h"
#include "../types/ContactMessage.h"
#include "../types/ChannelMessage.h"

namespace MeshCore {

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_messages.size());
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
        return {};
    }

    const MessageEntry &entry = m_messages.at(index.row());

    switch (role) {
    case MessageTypeRole:
        return static_cast<int>(entry.type);

    case SenderRole:
        if (entry.type == ContactMessageType) {
            auto msg = entry.data.value<ContactMessage>();
            return msg.senderPublicKeyPrefixHex();
        } else {
            auto msg = entry.data.value<ChannelMessage>();
            return QStringLiteral("Channel %1").arg(msg.channelIndex());
        }

    case TextRole:
        if (entry.type == ContactMessageType) {
            return entry.data.value<ContactMessage>().text();
        } else {
            return entry.data.value<ChannelMessage>().text();
        }

    case TimestampRole:
        if (entry.type == ContactMessageType) {
            return entry.data.value<ContactMessage>().senderTimestamp();
        } else {
            return entry.data.value<ChannelMessage>().senderTimestamp();
        }

    case DateTimeRole:
        if (entry.type == ContactMessageType) {
            return entry.data.value<ContactMessage>().dateTime();
        } else {
            return entry.data.value<ChannelMessage>().dateTime();
        }

    case IsDirectRole:
        if (entry.type == ChannelMessageType) {
            return entry.data.value<ChannelMessage>().isDirect();
        }
        return false;

    case PathLenRole:
        if (entry.type == ContactMessageType) {
            return entry.data.value<ContactMessage>().pathLen();
        } else {
            return entry.data.value<ChannelMessage>().pathLen();
        }

    case ChannelIndexRole:
        if (entry.type == ChannelMessageType) {
            return entry.data.value<ChannelMessage>().channelIndex();
        }
        return -1;

    case TextTypeRole:
        if (entry.type == ContactMessageType) {
            return static_cast<int>(entry.data.value<ContactMessage>().textType());
        } else {
            return static_cast<int>(entry.data.value<ChannelMessage>().textType());
        }

    default:
        return {};
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    static QHash<int, QByteArray> roles{
        {MessageTypeRole, "messageType"},
        {SenderRole, "sender"},
        {TextRole, "text"},
        {TimestampRole, "timestamp"},
        {DateTimeRole, "dateTime"},
        {IsDirectRole, "isDirect"},
        {PathLenRole, "pathLen"},
        {ChannelIndexRole, "channelIndex"},
        {TextTypeRole, "textType"}
    };
    return roles;
}

void MessageModel::clear()
{
    if (m_messages.isEmpty()) {
        return;
    }
    beginResetModel();
    m_messages.clear();
    endResetModel();
    Q_EMIT countChanged();
}

void MessageModel::addContactMessage(const ContactMessage &message)
{
    MessageEntry entry{ContactMessageType, QVariant::fromValue(message)};

    beginInsertRows(QModelIndex(), static_cast<int>(m_messages.size()), static_cast<int>(m_messages.size()));
    m_messages.append(entry);
    endInsertRows();
    Q_EMIT countChanged();
}

void MessageModel::addChannelMessage(const ChannelMessage &message)
{
    MessageEntry entry{ChannelMessageType, QVariant::fromValue(message)};

    beginInsertRows(QModelIndex(), static_cast<int>(m_messages.size()), static_cast<int>(m_messages.size()));
    m_messages.append(entry);
    endInsertRows();
    Q_EMIT countChanged();
}

} // namespace MeshCore
