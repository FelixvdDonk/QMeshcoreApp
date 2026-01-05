#include "ContactModel.h"

namespace MeshCore {

ContactModel::ContactModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ContactModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_contacts.size());
}

QVariant ContactModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_contacts.size()) {
        return {};
    }

    const Contact &contact = m_contacts.at(index.row());

    switch (role) {
    case PublicKeyRole:
        return contact.publicKey();
    case PublicKeyHexRole:
        return contact.publicKeyHex();
    case TypeRole:
        return static_cast<int>(contact.type());
    case TypeStringRole:
        return contact.typeString();
    case FlagsRole:
        return contact.flags();
    case OutPathLenRole:
        return contact.outPathLen();
    case NameRole:
        return contact.name();
    case LastAdvertRole:
        return contact.lastAdvert();
    case LatitudeRole:
        return contact.latitude();
    case LongitudeRole:
        return contact.longitude();
    case LatitudeDecimalRole:
        return contact.latitudeDecimal();
    case LongitudeDecimalRole:
        return contact.longitudeDecimal();
    case LastModifiedRole:
        return contact.lastModified();
    case ContactRole:
        return QVariant::fromValue(contact);
    default:
        return {};
    }
}

QHash<int, QByteArray> ContactModel::roleNames() const
{
    static QHash<int, QByteArray> roles{
        {PublicKeyRole, "publicKey"},
        {PublicKeyHexRole, "publicKeyHex"},
        {TypeRole, "type"},
        {TypeStringRole, "typeString"},
        {FlagsRole, "flags"},
        {OutPathLenRole, "outPathLen"},
        {NameRole, "name"},
        {LastAdvertRole, "lastAdvert"},
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
        {LatitudeDecimalRole, "latitudeDecimal"},
        {LongitudeDecimalRole, "longitudeDecimal"},
        {LastModifiedRole, "lastModified"},
        {ContactRole, "contact"}
    };
    return roles;
}

Contact ContactModel::get(int index) const
{
    if (index < 0 || index >= m_contacts.size()) {
        return {};
    }
    return m_contacts.at(index);
}

Contact ContactModel::findByName(const QString &name) const
{
    for (const auto &contact : m_contacts) {
        if (contact.name() == name) {
            return contact;
        }
    }
    return {};
}

Contact ContactModel::findByPublicKeyPrefix(const QByteArray &prefix) const
{
    for (const auto &contact : m_contacts) {
        if (contact.publicKey().startsWith(prefix)) {
            return contact;
        }
    }
    return {};
}

int ContactModel::indexOf(const QByteArray &publicKey) const
{
    for (int i = 0; i < m_contacts.size(); ++i) {
        if (m_contacts.at(i).publicKey() == publicKey) {
            return i;
        }
    }
    return -1;
}

void ContactModel::clear()
{
    if (m_contacts.isEmpty()) {
        return;
    }
    beginResetModel();
    m_contacts.clear();
    endResetModel();
    Q_EMIT countChanged();
}

void ContactModel::addContact(const Contact &contact)
{
    // Check if already exists
    int existingIdx = indexOf(contact.publicKey());
    if (existingIdx >= 0) {
        updateContact(contact);
        return;
    }

    beginInsertRows(QModelIndex(), static_cast<int>(m_contacts.size()), static_cast<int>(m_contacts.size()));
    m_contacts.append(contact);
    endInsertRows();
    Q_EMIT countChanged();
}

void ContactModel::addContacts(const QList<Contact> &contacts)
{
    if (contacts.isEmpty()) {
        return;
    }

    // Filter out duplicates
    QList<Contact> newContacts;
    for (const auto &contact : contacts) {
        if (indexOf(contact.publicKey()) < 0) {
            newContacts.append(contact);
        }
    }

    if (newContacts.isEmpty()) {
        return;
    }

    int first = static_cast<int>(m_contacts.size());
    int last = first + static_cast<int>(newContacts.size()) - 1;

    beginInsertRows(QModelIndex(), first, last);
    m_contacts.append(newContacts);
    endInsertRows();
    Q_EMIT countChanged();
}

void ContactModel::updateContact(const Contact &contact)
{
    int idx = indexOf(contact.publicKey());
    if (idx < 0) {
        addContact(contact);
        return;
    }

    m_contacts[idx] = contact;
    QModelIndex modelIdx = index(idx);
    Q_EMIT dataChanged(modelIdx, modelIdx);
}

void ContactModel::removeContact(const QByteArray &publicKey)
{
    int idx = indexOf(publicKey);
    if (idx < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), idx, idx);
    m_contacts.removeAt(idx);
    endRemoveRows();
    Q_EMIT countChanged();
}

} // namespace MeshCore
