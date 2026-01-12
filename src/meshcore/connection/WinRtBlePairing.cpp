// Include Qt header first to get Q_OS_WIN defined
#include <QtGlobal>

#ifdef Q_OS_WIN

#include "WinRtBlePairing.h"
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>

// Windows headers - must come after Qt headers
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Enumeration.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Enumeration;

namespace MeshCore {

class WinRtBlePairing::Private
{
public:
    bool initialized = false;
    
    // Convert QBluetoothAddress to uint64 for WinRT
    static uint64_t addressToUint64(const QBluetoothAddress &address) {
        QString addrStr = address.toString().remove(':');
        bool ok;
        uint64_t addr = addrStr.toULongLong(&ok, 16);
        return ok ? addr : 0;
    }
    
    // Get BLE device from address - must be called from worker thread
    static BluetoothLEDevice getDevice(const QBluetoothAddress &address) {
        uint64_t addr = addressToUint64(address);
        if (addr == 0) return nullptr;
        
        try {
            auto asyncOp = BluetoothLEDevice::FromBluetoothAddressAsync(addr);
            return asyncOp.get();
        } catch (...) {
            return nullptr;
        }
    }
    
    // Initialize WinRT apartment for worker thread
    static void initApartment() {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (const winrt::hresult_error&) {
            // Already initialized, that's OK
        }
    }
};

WinRtBlePairing::WinRtBlePairing(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    // Don't initialize WinRT here - do it in worker threads
    qDebug() << "WinRtBlePairing: Created";
}

WinRtBlePairing::~WinRtBlePairing()
{
    delete d;
}

bool WinRtBlePairing::isPaired(const QBluetoothAddress &address)
{
    try {
        auto device = Private::getDevice(address);
        if (!device) {
            qWarning() << "WinRT BLE: Could not find device" << address.toString();
            return false;
        }
        
        auto deviceInfo = device.DeviceInformation();
        auto pairing = deviceInfo.Pairing();
        return pairing.IsPaired();
    }
    catch (const winrt::hresult_error &e) {
        qWarning() << "WinRT BLE: Error checking pairing status:" << QString::fromStdWString(e.message().c_str());
        return false;
    }
}

bool WinRtBlePairing::unpair(const QBluetoothAddress &address)
{
    try {
        Q_EMIT pairingProgress(QStringLiteral("Finding device..."));
        
        auto device = Private::getDevice(address);
        if (!device) {
            Q_EMIT pairingFinished(address, false, QStringLiteral("Device not found"));
            return false;
        }
        
        auto deviceInfo = device.DeviceInformation();
        auto pairing = deviceInfo.Pairing();
        
        if (!pairing.IsPaired()) {
            Q_EMIT pairingProgress(QStringLiteral("Device is not paired"));
            Q_EMIT pairingFinished(address, true, QString());
            return true;
        }
        
        Q_EMIT pairingProgress(QStringLiteral("Unpairing device..."));
        
        auto result = pairing.UnpairAsync().get();
        
        bool success = (result.Status() == DeviceUnpairingResultStatus::Unpaired);
        
        if (success) {
            Q_EMIT pairingProgress(QStringLiteral("Device unpaired successfully"));
            Q_EMIT pairingFinished(address, true, QString());
        } else {
            QString error = QStringLiteral("Unpair failed: %1").arg(static_cast<int>(result.Status()));
            Q_EMIT pairingFinished(address, false, error);
        }
        
        return success;
    }
    catch (const winrt::hresult_error &e) {
        QString error = QString::fromStdWString(e.message().c_str());
        qWarning() << "WinRT BLE: Unpair error:" << error;
        Q_EMIT pairingFinished(address, false, error);
        return false;
    }
}

bool WinRtBlePairing::pairWithPin(const QBluetoothAddress &address, quint32 pin)
{
    m_pin = pin;
    
    try {
        Q_EMIT pairingProgress(QStringLiteral("Finding device..."));
        
        auto device = Private::getDevice(address);
        if (!device) {
            Q_EMIT pairingFinished(address, false, QStringLiteral("Device not found. Make sure you've scanned for it first."));
            return false;
        }
        
        qDebug() << "WinRT BLE: Found device:" << QString::fromStdWString(device.Name().c_str());
        Q_EMIT pairingProgress(QStringLiteral("Found device: %1").arg(QString::fromStdWString(device.Name().c_str())));
        
        auto deviceInfo = device.DeviceInformation();
        auto pairing = deviceInfo.Pairing();
        
        if (pairing.IsPaired()) {
            Q_EMIT pairingProgress(QStringLiteral("Device is already paired"));
            Q_EMIT pairingFinished(address, true, QString());
            return true;
        }
        
        if (!pairing.CanPair()) {
            Q_EMIT pairingFinished(address, false, QStringLiteral("Device does not support pairing"));
            return false;
        }
        
        // Get custom pairing for PIN support
        auto customPairing = pairing.Custom();
        
        // Set up the pairing requested handler
        auto pairingRequestedToken = customPairing.PairingRequested(
            [this, address, pin](DeviceInformationCustomPairing const&, DevicePairingRequestedEventArgs const& args) {
                qDebug() << "WinRT BLE: Pairing requested, kind:" << static_cast<int>(args.PairingKind());
                Q_EMIT pairingProgress(QStringLiteral("Pairing requested..."));
                
                switch (args.PairingKind()) {
                case DevicePairingKinds::ProvidePin:
                    qDebug() << "WinRT BLE: Providing PIN:" << pin;
                    Q_EMIT pairingProgress(QStringLiteral("Providing PIN: %1").arg(pin));
                    args.Accept(winrt::hstring(QString::number(pin).toStdWString()));
                    break;
                    
                case DevicePairingKinds::ConfirmOnly:
                    qDebug() << "WinRT BLE: Confirming pairing";
                    Q_EMIT pairingProgress(QStringLiteral("Confirming pairing..."));
                    args.Accept();
                    break;
                    
                case DevicePairingKinds::DisplayPin:
                    qDebug() << "WinRT BLE: Device displaying PIN";
                    Q_EMIT pairingProgress(QStringLiteral("Device is displaying PIN"));
                    args.Accept();
                    break;
                    
                case DevicePairingKinds::ConfirmPinMatch:
                    qDebug() << "WinRT BLE: Confirm PIN match";
                    Q_EMIT pairingProgress(QStringLiteral("Confirming PIN match..."));
                    args.Accept();
                    break;
                    
                default:
                    qDebug() << "WinRT BLE: Unknown pairing kind, accepting";
                    args.Accept();
                    break;
                }
            });
        
        Q_EMIT pairingProgress(QStringLiteral("Initiating pairing with PIN %1...").arg(pin));
        
        // Request pairing with all supported kinds
        auto pairingKinds = DevicePairingKinds::ConfirmOnly | 
                           DevicePairingKinds::ProvidePin |
                           DevicePairingKinds::ConfirmPinMatch |
                           DevicePairingKinds::DisplayPin;
        
        auto result = customPairing.PairAsync(pairingKinds).get();
        
        // Unregister the handler
        customPairing.PairingRequested(pairingRequestedToken);
        
        bool success = (result.Status() == DevicePairingResultStatus::Paired);
        
        if (success) {
            qDebug() << "WinRT BLE: Pairing successful!";
            Q_EMIT pairingProgress(QStringLiteral("Pairing successful!"));
            Q_EMIT pairingFinished(address, true, QString());
        } else {
            QString statusStr;
            switch (result.Status()) {
            case DevicePairingResultStatus::NotReadyToPair:
                statusStr = QStringLiteral("Device not ready to pair");
                break;
            case DevicePairingResultStatus::NotPaired:
                statusStr = QStringLiteral("Pairing was not completed");
                break;
            case DevicePairingResultStatus::AlreadyPaired:
                statusStr = QStringLiteral("Device is already paired");
                success = true;  // This is actually OK
                break;
            case DevicePairingResultStatus::ConnectionRejected:
                statusStr = QStringLiteral("Connection was rejected");
                break;
            case DevicePairingResultStatus::TooManyConnections:
                statusStr = QStringLiteral("Too many connections");
                break;
            case DevicePairingResultStatus::HardwareFailure:
                statusStr = QStringLiteral("Hardware failure");
                break;
            case DevicePairingResultStatus::AuthenticationTimeout:
                statusStr = QStringLiteral("Authentication timed out");
                break;
            case DevicePairingResultStatus::AuthenticationNotAllowed:
                statusStr = QStringLiteral("Authentication not allowed");
                break;
            case DevicePairingResultStatus::AuthenticationFailure:
                statusStr = QStringLiteral("Authentication failed - wrong PIN?");
                break;
            case DevicePairingResultStatus::NoSupportedProfiles:
                statusStr = QStringLiteral("No supported profiles");
                break;
            case DevicePairingResultStatus::ProtectionLevelCouldNotBeMet:
                statusStr = QStringLiteral("Required security level could not be met");
                break;
            case DevicePairingResultStatus::AccessDenied:
                statusStr = QStringLiteral("Access denied");
                break;
            case DevicePairingResultStatus::InvalidCeremonyData:
                statusStr = QStringLiteral("Invalid ceremony data");
                break;
            case DevicePairingResultStatus::PairingCanceled:
                statusStr = QStringLiteral("Pairing was canceled");
                break;
            case DevicePairingResultStatus::OperationAlreadyInProgress:
                statusStr = QStringLiteral("Pairing operation already in progress");
                break;
            case DevicePairingResultStatus::RequiredHandlerNotRegistered:
                statusStr = QStringLiteral("Required handler not registered");
                break;
            case DevicePairingResultStatus::RejectedByHandler:
                statusStr = QStringLiteral("Rejected by handler");
                break;
            case DevicePairingResultStatus::RemoteDeviceHasAssociation:
                statusStr = QStringLiteral("Remote device has existing association");
                break;
            case DevicePairingResultStatus::Failed:
            default:
                statusStr = QStringLiteral("Pairing failed (code: %1)").arg(static_cast<int>(result.Status()));
                break;
            }
            
            qWarning() << "WinRT BLE: Pairing failed:" << statusStr;
            Q_EMIT pairingFinished(address, success, statusStr);
        }
        
        return success;
    }
    catch (const winrt::hresult_error &e) {
        QString error = QString::fromStdWString(e.message().c_str());
        qWarning() << "WinRT BLE: Pairing error:" << error;
        Q_EMIT pairingFinished(address, false, error);
        return false;
    }
}

} // namespace MeshCore

#endif // Q_OS_WIN
