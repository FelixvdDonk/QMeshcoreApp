#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QVariant>

namespace MeshCore {

class ContactMessage;
class ChannelMessage;

/**
 * @brief Unified message model for both contact and channel messages
 */
class MessageModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum MessageType {
        ContactMessageType,
        ChannelMessageType
    };
    Q_ENUM(MessageType)

    enum Roles {
        MessageTypeRole = Qt::UserRole + 1,
        SenderRole,          // Public key prefix hex or "Channel X"
        TextRole,
        TimestampRole,
        DateTimeRole,
        IsDirectRole,
        PathLenRole,
        ChannelIndexRole,
        TextTypeRole
    };

    explicit MessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_messages.size()); }

    void clear();
    void addContactMessage(const ContactMessage &message);
    void addChannelMessage(const ChannelMessage &message);

Q_SIGNALS:
    void countChanged();

private:
    struct MessageEntry {
        MessageType type;
        QVariant data;
    };

    QList<MessageEntry> m_messages;
};

} // namespace MeshCore

#endif // MESSAGEMODEL_H
