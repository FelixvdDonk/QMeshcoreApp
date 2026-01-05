#ifndef CONTACTMODEL_H
#define CONTACTMODEL_H

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include "../types/Contact.h"

namespace MeshCore {

/**
 * @brief List model for contacts, suitable for QML ListView
 */
class ContactModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        PublicKeyRole = Qt::UserRole + 1,
        PublicKeyHexRole,
        TypeRole,
        TypeStringRole,
        FlagsRole,
        OutPathLenRole,
        NameRole,
        LastAdvertRole,
        LatitudeRole,
        LongitudeRole,
        LatitudeDecimalRole,
        LongitudeDecimalRole,
        LastModifiedRole,
        ContactRole  // Returns the whole Contact object
    };

    explicit ContactModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Count property
    int count() const { return static_cast<int>(m_contacts.size()); }

    // Data access
    Q_INVOKABLE MeshCore::Contact get(int index) const;
    Q_INVOKABLE MeshCore::Contact findByName(const QString &name) const;
    Q_INVOKABLE MeshCore::Contact findByPublicKeyPrefix(const QByteArray &prefix) const;
    Q_INVOKABLE int indexOf(const QByteArray &publicKey) const;

    // Data modification
    void clear();
    void addContact(const Contact &contact);
    void addContacts(const QList<Contact> &contacts);
    void updateContact(const Contact &contact);
    void removeContact(const QByteArray &publicKey);

Q_SIGNALS:
    void countChanged();

private:
    QList<Contact> m_contacts;
};

} // namespace MeshCore

#endif // CONTACTMODEL_H
