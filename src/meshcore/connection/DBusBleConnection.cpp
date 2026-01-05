#include "DBusBleConnection.h"
#include "BluezAgent.h"
#include "../MeshCoreConstants.h"
#include <QBluetoothAddress>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QRegularExpression>
#include <QThread>
#include <unistd.h>  // For write(), close()

namespace MeshCore {

// BlueZ DBus service and interfaces
static const QString BLUEZ_SERVICE = QStringLiteral("org.bluez");
static const QString BLUEZ_ADAPTER_IFACE = QStringLiteral("org.bluez.Adapter1");
static const QString BLUEZ_DEVICE_IFACE = QStringLiteral("org.bluez.Device1");
static const QString BLUEZ_GATT_SERVICE_IFACE = QStringLiteral("org.bluez.GattService1");
static const QString BLUEZ_GATT_CHAR_IFACE = QStringLiteral("org.bluez.GattCharacteristic1");
static const QString DBUS_PROPERTIES_IFACE = QStringLiteral("org.freedesktop.DBus.Properties");
static const QString DBUS_OBJECT_MANAGER_IFACE = QStringLiteral("org.freedesktop.DBus.ObjectManager");

// Nordic UART UUIDs (lowercase for BlueZ)
static const QString NUS_SERVICE_UUID = QStringLiteral("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static const QString NUS_RX_CHAR_UUID = QStringLiteral("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static const QString NUS_TX_CHAR_UUID = QStringLiteral("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

DBusBleConnection::DBusBleConnection(QObject *parent)
    : MeshCoreConnection(parent)
{
    // Create and register the BlueZ agent for PIN pairing
    m_agent = new BluezAgent(this);
    if (!m_agent->registerAgent()) {
        qWarning() << "DBus BLE: Failed to register BlueZ agent - pairing may not work automatically";
    }
}

DBusBleConnection::~DBusBleConnection()
{
    close();
    // Agent will be deleted by QObject parent-child relationship
    // but unregister it explicitly first
    if (m_agent) {
        m_agent->unregisterAgent();
    }
}

void DBusBleConnection::connectToDevice(const QBluetoothDeviceInfo &deviceInfo)
{
    // Store address first before any operations
    QString address = deviceInfo.address().toString();
    QString name = deviceInfo.name();
    
    qDebug() << "DBus BLE: Connecting to" << name << address;
    
    // Now close any existing connection (but don't emit disconnected if not connected)
    if (m_connectionTimer) {
        m_connectionTimer->stop();
        delete m_connectionTimer;
        m_connectionTimer = nullptr;
    }
    
    // Close file descriptors
    if (m_notifyNotifier) {
        delete m_notifyNotifier;
        m_notifyNotifier = nullptr;
    }
    if (m_writeFd >= 0) {
        ::close(m_writeFd);
        m_writeFd = -1;
    }
    if (m_notifyFd >= 0) {
        ::close(m_notifyFd);
        m_notifyFd = -1;
    }
    
    delete m_txCharInterface;
    m_txCharInterface = nullptr;
    delete m_rxCharInterface;
    m_rxCharInterface = nullptr;
    delete m_deviceInterface;
    m_deviceInterface = nullptr;
    
    m_devicePath.clear();
    m_rxCharPath.clear();
    m_txCharPath.clear();
    
    m_deviceInfo = deviceInfo;
    m_deviceAddress = address;  // Store separately for safety
    m_retryCount = 0;
    m_notificationsEnabled = false;
    m_writeOnlyMode = false;
    
    if (!findDevicePath()) {
        Q_EMIT errorOccurred(QStringLiteral("Could not find device in BlueZ. Make sure it's paired."));
        return;
    }
    
    // Set up the timer for characteristic discovery (started after connect completes)
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(true);
    connect(m_connectionTimer, &QTimer::timeout, this, [this]() {
        if (findCharacteristics()) {
            qDebug() << "DBus BLE: Characteristics found";
            
            // Subscribe to D-Bus property changes for receiving notifications
            subscribeToNotifications();
            
            // Acquire write fd asynchronously - will emit connected when done
            acquireWrite();
        } else if (m_retryCount < MaxRetries) {
            m_retryCount++;
            qDebug() << "DBus BLE: Retrying characteristic discovery..." << m_retryCount << "/" << MaxRetries;
            m_connectionTimer->start(300);  // Quick retry
        } else {
            Q_EMIT errorOccurred(QStringLiteral("Could not find GATT characteristics"));
        }
    });
    
    // Start async connection - timer will be started when connect completes
    if (!connectDevice()) {
        Q_EMIT errorOccurred(QStringLiteral("Failed to initiate connection to device"));
        return;
    }
    
    // If already connected, start characteristic discovery immediately
    if (m_deviceInterface) {
        QVariant connected = m_deviceInterface->property("Connected");
        if (connected.toBool()) {
            qDebug() << "DBus BLE: Device already connected, starting characteristic discovery";
            m_connectionTimer->start(100);  // Start quickly
        }
        // Otherwise, the async Connect() callback will start the timer
    }
}

void DBusBleConnection::close()
{
    if (m_connectionTimer) {
        m_connectionTimer->stop();
        delete m_connectionTimer;
        m_connectionTimer = nullptr;
    }
    
    // Clean up socket notifier first
    if (m_notifyNotifier) {
        m_notifyNotifier->setEnabled(false);
        delete m_notifyNotifier;
        m_notifyNotifier = nullptr;
    }
    
    // Close file descriptors
    if (m_writeFd >= 0) {
        ::close(m_writeFd);
        m_writeFd = -1;
    }
    if (m_notifyFd >= 0) {
        ::close(m_notifyFd);
        m_notifyFd = -1;
    }
    
    // Stop notifications if active (ignore errors - connection may already be gone)
    if (m_txCharInterface) {
        QDBusReply<void> reply = m_txCharInterface->call(QStringLiteral("StopNotify"));
        if (!reply.isValid()) {
            qDebug() << "DBus BLE: StopNotify failed (expected):" << reply.error().message();
        }
        delete m_txCharInterface;
        m_txCharInterface = nullptr;
    }
    
    delete m_rxCharInterface;
    m_rxCharInterface = nullptr;
    
    // Disconnect device
    if (m_deviceInterface) {
        QDBusReply<void> reply = m_deviceInterface->call(QStringLiteral("Disconnect"));
        if (!reply.isValid()) {
            qDebug() << "DBus BLE: Disconnect failed:" << reply.error().message();
        }
        delete m_deviceInterface;
        m_deviceInterface = nullptr;
    }
    
    m_devicePath.clear();
    m_rxCharPath.clear();
    m_txCharPath.clear();
    m_notificationsEnabled = false;
    m_writeOnlyMode = false;
    
    onDisconnected();
}

bool DBusBleConnection::findDevicePath()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (!bus.isConnected()) {
        qDebug() << "DBus BLE: System bus not connected";
        return false;
    }
    
    // Use stored address (safer than accessing m_deviceInfo.address())
    QString targetAddress = m_deviceAddress.toUpper().replace(':', '_');
    qDebug() << "DBus BLE: Looking for device with address pattern:" << targetAddress;
    
    QDBusMessage msg = QDBusMessage::createMethodCall(
        BLUEZ_SERVICE, QStringLiteral("/"),
        DBUS_OBJECT_MANAGER_IFACE, QStringLiteral("GetManagedObjects"));
    
    QDBusMessage response = bus.call(msg);
    if (response.type() != QDBusMessage::ReplyMessage) {
        qDebug() << "DBus BLE: GetManagedObjects call failed:" << response.errorMessage();
        return false;
    }
    
    if (response.arguments().isEmpty()) {
        qDebug() << "DBus BLE: Empty response from GetManagedObjects";
        return false;
    }
    
    // Parse the response - it's a{oa{sa{sv}}}
    // Use a simpler approach - iterate through all known adapter/device paths
    // BlueZ device paths follow pattern: /org/bluez/hci0/dev_XX_XX_XX_XX_XX_XX
    QString expectedPath = QStringLiteral("/org/bluez/hci0/dev_%1").arg(targetAddress);
    qDebug() << "DBus BLE: Expected device path:" << expectedPath;
    
    // Check if the device exists at this path
    QDBusInterface deviceCheck(BLUEZ_SERVICE, expectedPath, BLUEZ_DEVICE_IFACE, bus);
    if (deviceCheck.isValid()) {
        m_devicePath = expectedPath;
        qDebug() << "DBus BLE: Found device at path:" << m_devicePath;
        return true;
    }
    
    // Try hci1 as well
    expectedPath = QStringLiteral("/org/bluez/hci1/dev_%1").arg(targetAddress);
    QDBusInterface deviceCheck2(BLUEZ_SERVICE, expectedPath, BLUEZ_DEVICE_IFACE, bus);
    if (deviceCheck2.isValid()) {
        m_devicePath = expectedPath;
        qDebug() << "DBus BLE: Found device at path:" << m_devicePath;
        return true;
    }
    
    qDebug() << "DBus BLE: Device not found in BlueZ. Make sure it's paired.";
    return false;
}

void DBusBleConnection::setPin(quint32 pin)
{
    if (m_agent) {
        m_agent->setPin(pin);
        qDebug() << "DBus BLE: PIN set to" << pin;
    }
}

quint32 DBusBleConnection::pin() const
{
    return m_agent ? m_agent->pin() : 123456;
}

bool DBusBleConnection::ensurePaired()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (!m_deviceInterface || !m_deviceInterface->isValid()) {
        qDebug() << "DBus BLE: Cannot check pairing - device interface not valid";
        return false;
    }
    
    // Check if already paired
    QVariant paired = m_deviceInterface->property("Paired");
    qDebug() << "DBus BLE: Device Paired property:" << paired.toBool();
    
    if (paired.toBool()) {
        qDebug() << "DBus BLE: Device already paired";
        return true;
    }
    
    // Device not paired - initiate pairing
    qDebug() << "DBus BLE: Device not paired, initiating pairing with PIN" << pin();
    
    // Make sure agent is registered
    if (m_agent && !m_agent->isRegistered()) {
        m_agent->registerAgent();
    }
    
    // Call Pair() method on Device1 interface
    QDBusMessage pairMsg = QDBusMessage::createMethodCall(
        BLUEZ_SERVICE, m_devicePath, BLUEZ_DEVICE_IFACE, QStringLiteral("Pair"));
    
    // Pairing can take a while - use 30 second timeout
    QDBusMessage reply = bus.call(pairMsg, QDBus::Block, 30000);
    
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QString error = reply.errorMessage();
        qDebug() << "DBus BLE: Pair() failed:" << error;
        
        // Check if it's "Already Paired" error (which is fine)
        if (error.contains(QStringLiteral("AlreadyExists")) || 
            error.contains(QStringLiteral("Already"))) {
            qDebug() << "DBus BLE: Device was already paired";
            return true;
        }
        return false;
    }
    
    qDebug() << "DBus BLE: Pairing successful!";
    
    // Trust the device so we don't need to pair again
    QDBusInterface props(BLUEZ_SERVICE, m_devicePath,
                         QStringLiteral("org.freedesktop.DBus.Properties"), bus);
    props.call(QStringLiteral("Set"), BLUEZ_DEVICE_IFACE, 
               QStringLiteral("Trusted"), QVariant::fromValue(QDBusVariant(true)));
    qDebug() << "DBus BLE: Device trusted";
    
    return true;
}

bool DBusBleConnection::connectDevice()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    m_deviceInterface = new QDBusInterface(BLUEZ_SERVICE, m_devicePath,
                                            BLUEZ_DEVICE_IFACE, bus, this);
    
    if (!m_deviceInterface->isValid()) {
        qDebug() << "DBus BLE: Invalid device interface";
        return false;
    }
    
    // Subscribe to device property changes to detect disconnects
    bus.connect(BLUEZ_SERVICE, m_devicePath,
                QStringLiteral("org.freedesktop.DBus.Properties"),
                QStringLiteral("PropertiesChanged"),
                this, SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
    
    // Ensure device is paired (will initiate pairing with PIN if needed)
    if (!ensurePaired()) {
        qWarning() << "DBus BLE: Pairing failed - connection may not work";
        // Continue anyway - device might work without explicit pairing
    }
    
    // Check if already connected
    QVariant connected = m_deviceInterface->property("Connected");
    qDebug() << "DBus BLE: Device Connected property:" << connected.toBool();
    
    if (!connected.toBool()) {
        // Connect to device asynchronously
        qDebug() << "DBus BLE: Calling Connect() asynchronously...";
        QDBusMessage connectMsg = QDBusMessage::createMethodCall(
            BLUEZ_SERVICE, m_devicePath, BLUEZ_DEVICE_IFACE, QStringLiteral("Connect"));
        
        QDBusPendingCall pendingCall = bus.asyncCall(connectMsg, 30000);  // 30 second timeout
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
        
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
            w->deleteLater();
            
            QDBusPendingReply<> reply = *w;
            if (reply.isError()) {
                qDebug() << "DBus BLE: Connect failed:" << reply.error().message();
                Q_EMIT errorOccurred(QStringLiteral("BLE Connect failed: %1").arg(reply.error().message()));
                return;
            }
            
            qDebug() << "DBus BLE: Connect() succeeded";
            
            // Verify we're connected
            if (m_deviceInterface) {
                QVariant connected = m_deviceInterface->property("Connected");
                qDebug() << "DBus BLE: After connect, Connected:" << connected.toBool();
                
                if (!connected.toBool()) {
                    Q_EMIT errorOccurred(QStringLiteral("Device not connected after Connect()"));
                    return;
                }
            }
            
            // Now start the characteristic discovery timer
            if (m_connectionTimer) {
                m_connectionTimer->start(100);  // Start quickly after connect
            }
        });
        
        return true;  // Will continue asynchronously
    } else {
        qDebug() << "DBus BLE: Already connected, using existing connection";
        return true;
    }
}

bool DBusBleConnection::findCharacteristics()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    // Check if services are resolved
    if (m_deviceInterface) {
        QVariant resolved = m_deviceInterface->property("ServicesResolved");
        qDebug() << "DBus BLE: ServicesResolved:" << resolved.toBool();
        if (!resolved.toBool()) {
            return false;  // Retry later
        }
    }
    
    qDebug() << "DBus BLE: Searching for characteristics under" << m_devicePath;
    
    // Use introspection to find all child paths
    QDBusInterface introspect(BLUEZ_SERVICE, m_devicePath,
                               QStringLiteral("org.freedesktop.DBus.Introspectable"), bus);
    QDBusReply<QString> introReply = introspect.call(QStringLiteral("Introspect"));
    
    if (!introReply.isValid()) {
        qDebug() << "DBus BLE: Introspect failed:" << introReply.error().message();
        return false;
    }
    
    QString xml = introReply.value();
    qDebug() << "DBus BLE: Device introspection:" << xml.left(500);
    
    // Parse XML to find service nodes - look for <node name="serviceXXXX"/>
    QRegularExpression serviceRe(QStringLiteral("<node name=\"(service[0-9a-f]+)\"/>"));
    QRegularExpressionMatchIterator serviceMatches = serviceRe.globalMatch(xml);
    
    while (serviceMatches.hasNext()) {
        QRegularExpressionMatch match = serviceMatches.next();
        QString serviceName = match.captured(1);
        QString servicePath = QStringLiteral("%1/%2").arg(m_devicePath, serviceName);
        
        qDebug() << "DBus BLE: Found service:" << servicePath;
        
        // Get the service UUID
        QDBusInterface svcIface(BLUEZ_SERVICE, servicePath, BLUEZ_GATT_SERVICE_IFACE, bus);
        if (svcIface.isValid()) {
            QVariant svcUuid = svcIface.property("UUID");
            qDebug() << "DBus BLE: Service UUID:" << svcUuid.toString();
        }
        
        // Introspect the service to find characteristics
        QDBusInterface svcIntrospect(BLUEZ_SERVICE, servicePath,
                                      QStringLiteral("org.freedesktop.DBus.Introspectable"), bus);
        QDBusReply<QString> svcIntroReply = svcIntrospect.call(QStringLiteral("Introspect"));
        
        if (!svcIntroReply.isValid()) {
            continue;
        }
        
        QString svcXml = svcIntroReply.value();
        QRegularExpression charRe(QStringLiteral("<node name=\"(char[0-9a-f]+)\"/>"));
        QRegularExpressionMatchIterator charMatches = charRe.globalMatch(svcXml);
        
        while (charMatches.hasNext()) {
            QRegularExpressionMatch charMatch = charMatches.next();
            QString charName = charMatch.captured(1);
            QString charPath = QStringLiteral("%1/%2").arg(servicePath, charName);
            
            QDBusInterface charIface(BLUEZ_SERVICE, charPath, BLUEZ_GATT_CHAR_IFACE, bus);
            if (!charIface.isValid()) {
                continue;
            }
            
            QVariant uuidVar = charIface.property("UUID");
            if (!uuidVar.isValid()) {
                continue;
            }
            
            QString uuid = uuidVar.toString().toLower();
            qDebug() << "DBus BLE: Found characteristic" << uuid << "at" << charPath;
            
            if (uuid == NUS_RX_CHAR_UUID) {
                m_rxCharPath = charPath;
                qDebug() << "DBus BLE: Found RX characteristic";
            } else if (uuid == NUS_TX_CHAR_UUID) {
                m_txCharPath = charPath;
                qDebug() << "DBus BLE: Found TX characteristic";
            }
        }
    }
    
    if (m_rxCharPath.isEmpty() || m_txCharPath.isEmpty()) {
        qDebug() << "DBus BLE: Missing characteristics - RX:" << m_rxCharPath << "TX:" << m_txCharPath;
        return false;
    }
    
    // Create interfaces for the characteristics
    m_rxCharInterface = new QDBusInterface(BLUEZ_SERVICE, m_rxCharPath,
                                            BLUEZ_GATT_CHAR_IFACE, bus, this);
    m_txCharInterface = new QDBusInterface(BLUEZ_SERVICE, m_txCharPath,
                                            BLUEZ_GATT_CHAR_IFACE, bus, this);
    
    return m_rxCharInterface->isValid() && m_txCharInterface->isValid();
}

bool DBusBleConnection::acquireWrite()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (m_rxCharPath.isEmpty()) {
        qDebug() << "DBus BLE: acquireWrite - RX char path not set";
        return false;
    }
    
    qDebug() << "DBus BLE: Calling AcquireWrite on" << m_rxCharPath;
    
    // AcquireWrite returns (fd, mtu) - use variant map for options
    QDBusMessage msg = QDBusMessage::createMethodCall(
        BLUEZ_SERVICE, m_rxCharPath, BLUEZ_GATT_CHAR_IFACE, QStringLiteral("AcquireWrite"));
    
    QVariantMap options;
    msg << options;
    
    // Use async call to avoid blocking
    QDBusPendingCall pendingCall = bus.asyncCall(msg, 5000);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        
        QDBusPendingReply<QDBusUnixFileDescriptor, quint16> reply = *w;
        if (reply.isError()) {
            qDebug() << "DBus BLE: AcquireWrite failed:" << reply.error().message();
            // Still emit connected - we can try WriteValue fallback
            onConnected();
            return;
        }
        
        QDBusUnixFileDescriptor dbusfd = reply.argumentAt<0>();
        if (!dbusfd.isValid()) {
            qDebug() << "DBus BLE: AcquireWrite - invalid fd received";
            onConnected();
            return;
        }
        
        // Take ownership of fd
        m_writeFd = ::dup(dbusfd.fileDescriptor());
        m_writeMtu = reply.argumentAt<1>();
        
        qDebug() << "DBus BLE: AcquireWrite succeeded - fd=" << m_writeFd << "mtu=" << m_writeMtu;
        
        // Try AcquireNotify for receiving data (also async)
        tryAcquireNotify();
        
        // Emit connected immediately
        onConnected();
    });
    
    return true;  // Async - will complete later
}

bool DBusBleConnection::tryAcquireNotify()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (m_txCharPath.isEmpty()) {
        qDebug() << "DBus BLE: tryAcquireNotify - TX char path not set";
        return false;
    }
    
    qDebug() << "DBus BLE: Calling AcquireNotify (async) on" << m_txCharPath;
    
    // Use ASYNC call to avoid blocking the event loop
    // Blocking calls prevent us from processing property changes and can cause disconnections
    QDBusMessage msg = QDBusMessage::createMethodCall(
        BLUEZ_SERVICE, m_txCharPath, BLUEZ_GATT_CHAR_IFACE, QStringLiteral("AcquireNotify"));
    
    QVariantMap options;
    msg << options;
    
    // Use short timeout (2s) - if this fails, we fall back to property changes
    QDBusPendingCall pendingCall = bus.asyncCall(msg, 2000);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QDBusUnixFileDescriptor, quint16> reply = *w;
        if (reply.isError()) {
            qDebug() << "DBus BLE: AcquireNotify async failed:" << reply.error().message();
            // That's OK - we're already subscribed to property changes
        } else {
            QDBusUnixFileDescriptor dbusfd = reply.argumentAt<0>();
            if (dbusfd.isValid()) {
                m_notifyFd = ::dup(dbusfd.fileDescriptor());
                m_notifyMtu = reply.argumentAt<1>();
                qDebug() << "DBus BLE: AcquireNotify async succeeded - fd=" << m_notifyFd << "mtu=" << m_notifyMtu;
                
                // Set up socket notifier to read from notify fd
                if (m_notifyFd >= 0) {
                    m_notifyNotifier = new QSocketNotifier(m_notifyFd, QSocketNotifier::Read, this);
                    connect(m_notifyNotifier, &QSocketNotifier::activated, this, &DBusBleConnection::onNotifyFdReadyRead);
                    m_notificationsEnabled = true;
                    Q_EMIT notificationsEnabledChanged();
                }
            }
        }
        w->deleteLater();
    });
    
    // Return true to indicate we initiated the call (actual result comes async)
    return true;
}

void DBusBleConnection::subscribeToNotifications()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (m_txCharPath.isEmpty()) {
        qDebug() << "DBus BLE: Cannot subscribe - TX char path not set";
        return;
    }
    
    // Subscribe to property changes on the TX characteristic to receive data
    bool connected = bus.connect(BLUEZ_SERVICE, m_txCharPath,
                QStringLiteral("org.freedesktop.DBus.Properties"),
                QStringLiteral("PropertiesChanged"),
                this, SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
    
    qDebug() << "DBus BLE: Subscribed to TX characteristic property changes:" << connected;
}

void DBusBleConnection::checkNotificationsEnabled()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (m_txCharPath.isEmpty()) {
        return;
    }
    
    // Check if notifications are already enabled (from previous session)
    QDBusInterface txProps(BLUEZ_SERVICE, m_txCharPath, DBUS_PROPERTIES_IFACE, bus);
    QDBusReply<QVariant> notifyingReply = txProps.call(
        QStringLiteral("Get"), BLUEZ_GATT_CHAR_IFACE, QStringLiteral("Notifying"));
    
    if (notifyingReply.isValid()) {
        bool notifying = notifyingReply.value().toBool();
        qDebug() << "DBus BLE: Notifying property:" << notifying;
        if (notifying) {
            m_notificationsEnabled = true;
            Q_EMIT notificationsEnabledChanged();
        }
    }
}

void DBusBleConnection::tryStartNotifySync()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (m_txCharPath.isEmpty()) {
        qDebug() << "DBus BLE: Cannot start notify - TX char path not set";
        return;
    }
    
    // First check if notifications are already enabled
    QDBusInterface txProps(BLUEZ_SERVICE, m_txCharPath, DBUS_PROPERTIES_IFACE, bus);
    QDBusReply<QVariant> notifyingReply = txProps.call(
        QStringLiteral("Get"), BLUEZ_GATT_CHAR_IFACE, QStringLiteral("Notifying"));
    
    if (notifyingReply.isValid() && notifyingReply.value().toBool()) {
        qDebug() << "DBus BLE: Notifications already enabled, skipping StartNotify";
        m_notificationsEnabled = true;
        Q_EMIT notificationsEnabledChanged();
        return;
    }
    
    qDebug() << "DBus BLE: Calling StartNotify...";
    
    QDBusMessage msg = QDBusMessage::createMethodCall(
        BLUEZ_SERVICE, m_txCharPath, BLUEZ_GATT_CHAR_IFACE, QStringLiteral("StartNotify"));
    
    // Use a short timeout 
    QDBusMessage reply = bus.call(msg, QDBus::Block, 2000);
    
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "DBus BLE: StartNotify failed:" << reply.errorMessage();
        // Don't treat this as fatal - device might still work
    } else {
        qDebug() << "DBus BLE: StartNotify succeeded";
        m_notificationsEnabled = true;
        Q_EMIT notificationsEnabledChanged();
    }
}

void DBusBleConnection::tryStartNotify()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    qDebug() << "DBus BLE: Calling StartNotify (async)...";
    
    QDBusMessage msg = QDBusMessage::createMethodCall(
        BLUEZ_SERVICE, m_txCharPath, BLUEZ_GATT_CHAR_IFACE, QStringLiteral("StartNotify"));
    
    QDBusPendingCall pendingCall = bus.asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<> reply = *w;
        if (reply.isError()) {
            qDebug() << "DBus BLE: StartNotify failed:" << reply.error().message();
            // Don't emit error - this is expected on some devices
        } else {
            qDebug() << "DBus BLE: StartNotify succeeded";
            m_notificationsEnabled = true;
            Q_EMIT notificationsEnabledChanged();
        }
        w->deleteLater();
    });
}

void DBusBleConnection::onNotifyFdReadyRead()
{
    if (m_notifyFd < 0) return;
    
    // Read data from the notification fd
    char buffer[512];
    ssize_t bytesRead = ::read(m_notifyFd, buffer, sizeof(buffer));
    
    if (bytesRead > 0) {
        QByteArray data(buffer, bytesRead);
        qDebug() << "DBus BLE: Received via fd:" << data.size() << "bytes:" << data.toHex();
        onFrameReceived(data);
    } else if (bytesRead == 0) {
        qDebug() << "DBus BLE: Notify fd closed";
        // Device disconnected
    } else {
        qDebug() << "DBus BLE: Error reading from notify fd:" << errno;
    }
}

void DBusBleConnection::onWriteFdReadyRead()
{
    // Write fd shouldn't have data to read, but handle it anyway
    if (m_writeFd < 0) return;
    
    char buffer[64];
    ::read(m_writeFd, buffer, sizeof(buffer));
}

void DBusBleConnection::onPropertiesChanged(const QString &interface,
                                             const QVariantMap &changed,
                                             const QStringList &invalidated)
{
    Q_UNUSED(invalidated)
    
    // Handle Device1 property changes (connection state)
    if (interface == BLUEZ_DEVICE_IFACE) {
        if (changed.contains(QStringLiteral("Connected"))) {
            bool connected = changed.value(QStringLiteral("Connected")).toBool();
            qDebug() << "DBus BLE: Device Connected changed to:" << connected;
            if (!connected && m_connected) {
                qDebug() << "DBus BLE: Device disconnected!";
                onDisconnected();
            }
        }
        if (changed.contains(QStringLiteral("ServicesResolved"))) {
            bool resolved = changed.value(QStringLiteral("ServicesResolved")).toBool();
            qDebug() << "DBus BLE: ServicesResolved changed to:" << resolved;
        }
        return;
    }
    
    // Handle characteristic Value changes (notifications)
    if (interface == BLUEZ_GATT_CHAR_IFACE) {
        if (changed.contains(QStringLiteral("Value"))) {
            qDebug() << "DBus BLE: *** NOTIFICATION RECEIVED VIA PROPERTY CHANGE! ***";
            // BlueZ sends Value as an array of bytes (ay in D-Bus)
            QVariant valueVar = changed.value(QStringLiteral("Value"));
            QByteArray data;
            
            // Try to extract as QByteArray first
            if (valueVar.canConvert<QByteArray>()) {
                data = valueVar.toByteArray();
            } else if (valueVar.typeId() == QMetaType::QVariantList) {
                // May come as a list of variants (each containing a byte)
                QVariantList list = valueVar.toList();
                for (const QVariant &v : list) {
                    data.append(static_cast<char>(v.toInt()));
                }
            } else {
                qDebug() << "DBus BLE: Unknown Value type:" << valueVar.typeName();
            }
            
            if (!data.isEmpty()) {
                qDebug() << "DBus BLE: Received notification" << data.size() << "bytes:" << data.toHex();
                onFrameReceived(data);
            }
        }
        if (changed.contains(QStringLiteral("Notifying"))) {
            bool notifying = changed.value(QStringLiteral("Notifying")).toBool();
            qDebug() << "DBus BLE: Notifying property changed to:" << notifying;
            m_notificationsEnabled = notifying;
            Q_EMIT notificationsEnabledChanged();
        }
        return;
    }
}

void DBusBleConnection::sendToRadioFrame(const QByteArray &frame)
{
    Q_EMIT frameSent(frame);
    writeToDevice(frame);
}

void DBusBleConnection::writeToDevice(const QByteArray &data)
{
    // Check if device is still connected
    if (m_deviceInterface) {
        QVariant connected = m_deviceInterface->property("Connected");
        if (!connected.toBool()) {
            qWarning() << "DBus BLE: Device not connected, cannot write";
            Q_EMIT errorOccurred(QStringLiteral("Device disconnected"));
            return;
        }
    }
    
    // Prefer fd-based writing (more reliable)
    if (m_writeFd >= 0) {
        writeViaFd(data);
    } else {
        writeViaDBus(data);
    }
}

void DBusBleConnection::writeViaFd(const QByteArray &data)
{
    if (m_writeFd < 0) {
        qWarning() << "DBus BLE: Write fd not available";
        writeViaDBus(data);  // Fallback
        return;
    }
    
    qDebug() << "DBus BLE: Writing" << data.size() << "bytes via fd:" << data.toHex();
    
    // Respect MTU - split data if needed
    int offset = 0;
    while (offset < data.size()) {
        int chunkSize = qMin(static_cast<int>(m_writeMtu), data.size() - offset);
        ssize_t written = ::write(m_writeFd, data.constData() + offset, chunkSize);
        
        if (written < 0) {
            qWarning() << "DBus BLE: Write failed, errno=" << errno;
            Q_EMIT errorOccurred(QStringLiteral("Write failed"));
            return;
        }
        
        offset += written;
        
        // Small delay between chunks for BLE
        if (offset < data.size()) {
            QThread::msleep(10);
        }
    }
    
    qDebug() << "DBus BLE: Write succeeded (" << data.size() << " bytes)";
}

void DBusBleConnection::writeViaDBus(const QByteArray &data)
{
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (m_rxCharPath.isEmpty()) {
        qWarning() << "DBus BLE: Cannot write - RX characteristic not ready";
        return;
    }
    
    qDebug() << "DBus BLE: Writing" << data.size() << "bytes via DBus:" << data.toHex();
    
    // WriteValue takes a byte array and options dict
    QDBusMessage msg = QDBusMessage::createMethodCall(
        BLUEZ_SERVICE, m_rxCharPath, BLUEZ_GATT_CHAR_IFACE, QStringLiteral("WriteValue"));
    
    QVariantMap options;
    options[QStringLiteral("type")] = QStringLiteral("command");  // write-without-response
    
    msg << QVariant::fromValue(data) << options;
    
    // Note: WriteValue may timeout on some devices - use short timeout
    QDBusMessage reply = bus.call(msg, QDBus::Block, 3000);
    
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "DBus BLE: WriteValue failed:" << reply.errorMessage();
        Q_EMIT errorOccurred(QStringLiteral("Write failed: %1").arg(reply.errorMessage()));
    } else {
        qDebug() << "DBus BLE: Write succeeded";
    }
}

} // namespace MeshCore
