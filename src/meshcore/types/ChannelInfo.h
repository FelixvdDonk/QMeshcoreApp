#ifndef CHANNELINFO_H
#define CHANNELINFO_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace MeshCore {

/**
 * @brief Represents a channel configuration
 */
class ChannelInfo
{
    Q_GADGET

    Q_PROPERTY(quint8 index READ index CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QByteArray secret READ secret CONSTANT)
    Q_PROPERTY(QString secretHex READ secretHex CONSTANT)
    Q_PROPERTY(bool isEmpty READ isEmpty CONSTANT)

public:
    ChannelInfo() = default;
    ChannelInfo(quint8 index, const QString &name, const QByteArray &secret);

    [[nodiscard]] quint8 index() const { return m_index; }
    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] QByteArray secret() const { return m_secret; }
    [[nodiscard]] QString secretHex() const;
    [[nodiscard]] bool isEmpty() const;

private:
    quint8 m_index = 0;
    QString m_name;
    QByteArray m_secret;  // 16 bytes
};

} // namespace MeshCore

Q_DECLARE_METATYPE(MeshCore::ChannelInfo)

#endif // CHANNELINFO_H
