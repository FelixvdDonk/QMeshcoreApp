# PowerShell script to pair a BLE device on Windows
# Usage: .\pair-ble-device.ps1 -DeviceAddress "C0:7C:DC:97:FD:80"
#        .\pair-ble-device.ps1 -DeviceAddress "C0:7C:DC:97:FD:80" -Unpair
# Note: This requires running PowerShell as Administrator

param(
    [Parameter(Mandatory=$true)]
    [string]$DeviceAddress,
    
    [string]$Pin = "123456",
    
    [switch]$Unpair
)

Write-Host "BLE Device Tool for: $DeviceAddress" -ForegroundColor Cyan
if ($Unpair) {
    Write-Host "Mode: UNPAIR" -ForegroundColor Yellow
} else {
    Write-Host "Mode: PAIR (PIN: $Pin)" -ForegroundColor Yellow
}

# Load Windows Runtime assemblies
Add-Type -AssemblyName System.Runtime.WindowsRuntime

# Helper function to await WinRT async operations
$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | 
    Where-Object { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]

Function Await($WinRtTask, $ResultType) {
    $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
    $netTask = $asTask.Invoke($null, @($WinRtTask))
    $netTask.Wait(-1) | Out-Null
    $netTask.Result
}

Function AwaitAction($WinRtAction) {
    $asTask = [System.WindowsRuntimeSystemExtensions].GetMethod('AsTask', [Type[]]@([Windows.Foundation.IAsyncAction]))
    $netTask = $asTask.Invoke($null, @($WinRtAction))
    $netTask.Wait(-1) | Out-Null
}

try {
    # Load Bluetooth APIs
    [Windows.Devices.Bluetooth.BluetoothLEDevice, Windows.Devices.Bluetooth, ContentType=WindowsRuntime] | Out-Null
    [Windows.Devices.Enumeration.DevicePairingKinds, Windows.Devices.Enumeration, ContentType=WindowsRuntime] | Out-Null
    [Windows.Devices.Enumeration.DevicePairingResultStatus, Windows.Devices.Enumeration, ContentType=WindowsRuntime] | Out-Null
    [Windows.Devices.Enumeration.DeviceInformationPairing, Windows.Devices.Enumeration, ContentType=WindowsRuntime] | Out-Null

    # Convert MAC address to UInt64 for BLE
    $addressParts = $DeviceAddress -split ':'
    $addressUInt64 = [Convert]::ToUInt64(($addressParts -join ''), 16)
    
    Write-Host "Looking for BLE device..." -ForegroundColor Gray
    
    # Get the BLE device
    $bleDevice = Await ([Windows.Devices.Bluetooth.BluetoothLEDevice]::FromBluetoothAddressAsync($addressUInt64)) ([Windows.Devices.Bluetooth.BluetoothLEDevice])
    
    if ($null -eq $bleDevice) {
        Write-Host "ERROR: Could not find BLE device. Make sure:" -ForegroundColor Red
        Write-Host "  1. The device is powered on and in range" -ForegroundColor Red
        Write-Host "  2. The MAC address is correct" -ForegroundColor Red
        Write-Host "  3. You've scanned for the device recently (run your app's BLE scan first)" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Found device: $($bleDevice.Name)" -ForegroundColor Green
    Write-Host "Connection status: $($bleDevice.ConnectionStatus)" -ForegroundColor Gray
    
    # Check current pairing status
    $deviceInfo = $bleDevice.DeviceInformation
    $pairingInfo = $deviceInfo.Pairing
    
    Write-Host "Is paired: $($pairingInfo.IsPaired)" -ForegroundColor Gray
    Write-Host "Can pair: $($pairingInfo.CanPair)" -ForegroundColor Gray
    
    # Handle unpair request
    if ($Unpair) {
        if (-not $pairingInfo.IsPaired) {
            Write-Host "Device is not paired, nothing to unpair." -ForegroundColor Yellow
            exit 0
        }
        
        Write-Host "Unpairing device..." -ForegroundColor Yellow
        $unpairResult = Await ($pairingInfo.UnpairAsync()) ([Windows.Devices.Enumeration.DeviceUnpairingResult])
        
        if ($unpairResult.Status -eq [Windows.Devices.Enumeration.DeviceUnpairingResultStatus]::Unpaired) {
            Write-Host "SUCCESS: Device unpaired!" -ForegroundColor Green
            Write-Host "Now run without -Unpair to pair with PIN." -ForegroundColor Cyan
        } else {
            Write-Host "Unpair failed with status: $($unpairResult.Status)" -ForegroundColor Red
        }
        
        $bleDevice.Dispose()
        exit 0
    }
    
    if ($pairingInfo.IsPaired) {
        Write-Host "Device is already paired!" -ForegroundColor Green
        Write-Host "If notifications aren't working, try: .\pair-ble-device.ps1 -DeviceAddress `"$DeviceAddress`" -Unpair" -ForegroundColor Yellow
        Write-Host "Then pair again with PIN." -ForegroundColor Yellow
        exit 0
    }
    
    if (-not $pairingInfo.CanPair) {
        Write-Host "ERROR: Device does not support pairing or pairing is not available" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Initiating pairing..." -ForegroundColor Cyan
    
    # Try pairing with different protection levels
    [Windows.Devices.Enumeration.DevicePairingProtectionLevel, Windows.Devices.Enumeration, ContentType=WindowsRuntime] | Out-Null
    
    # Method 1: Try with no protection (simplest)
    Write-Host "Attempting pairing with no protection level..." -ForegroundColor Gray
    $pairingResult = Await ($pairingInfo.PairAsync([Windows.Devices.Enumeration.DevicePairingProtectionLevel]::None)) ([Windows.Devices.Enumeration.DevicePairingResult])
    Write-Host "Result: $($pairingResult.Status)" -ForegroundColor $(if ($pairingResult.Status -eq [Windows.Devices.Enumeration.DevicePairingResultStatus]::Paired) { 'Green' } else { 'Yellow' })
    
    if ($pairingResult.Status -eq [Windows.Devices.Enumeration.DevicePairingResultStatus]::Paired) {
        Write-Host "SUCCESS: Device paired!" -ForegroundColor Green
        $bleDevice.Dispose()
        exit 0
    }
    
    # Method 2: Try with encryption
    Write-Host "Attempting pairing with encryption..." -ForegroundColor Gray
    $pairingResult2 = Await ($pairingInfo.PairAsync([Windows.Devices.Enumeration.DevicePairingProtectionLevel]::Encryption)) ([Windows.Devices.Enumeration.DevicePairingResult])
    Write-Host "Result: $($pairingResult2.Status)" -ForegroundColor $(if ($pairingResult2.Status -eq [Windows.Devices.Enumeration.DevicePairingResultStatus]::Paired) { 'Green' } else { 'Yellow' })
    
    if ($pairingResult2.Status -eq [Windows.Devices.Enumeration.DevicePairingResultStatus]::Paired) {
        Write-Host "SUCCESS: Device paired with encryption!" -ForegroundColor Green
        $bleDevice.Dispose()
        exit 0
    }
    
    # Method 3: Try with EncryptionAndAuthentication
    Write-Host "Attempting pairing with encryption and authentication..." -ForegroundColor Gray
    $pairingResult3 = Await ($pairingInfo.PairAsync([Windows.Devices.Enumeration.DevicePairingProtectionLevel]::EncryptionAndAuthentication)) ([Windows.Devices.Enumeration.DevicePairingResult])
    Write-Host "Result: $($pairingResult3.Status)" -ForegroundColor $(if ($pairingResult3.Status -eq [Windows.Devices.Enumeration.DevicePairingResultStatus]::Paired) { 'Green' } else { 'Yellow' })
    
    if ($pairingResult3.Status -eq [Windows.Devices.Enumeration.DevicePairingResultStatus]::Paired) {
        Write-Host "SUCCESS: Device paired with encryption and authentication!" -ForegroundColor Green
        $bleDevice.Dispose()
        exit 0
    }
    
    # Method 4: Try basic pairing (default)
    Write-Host "Attempting default pairing..." -ForegroundColor Gray
    $pairingResult4 = Await ($pairingInfo.PairAsync()) ([Windows.Devices.Enumeration.DevicePairingResult])
    Write-Host "Result: $($pairingResult4.Status)" -ForegroundColor $(if ($pairingResult4.Status -eq [Windows.Devices.Enumeration.DevicePairingResultStatus]::Paired) { 'Green' } else { 'Yellow' })
    
    if ($pairingResult4.Status -eq [Windows.Devices.Enumeration.DevicePairingResultStatus]::Paired) {
        Write-Host "SUCCESS: Device paired!" -ForegroundColor Green
        $bleDevice.Dispose()
        exit 0
    }
    
    # All methods failed
    Write-Host "" -ForegroundColor White
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "ALL PAIRING METHODS FAILED" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "" -ForegroundColor White
    Write-Host "The MeshCore device requires authenticated pairing with PIN," -ForegroundColor Yellow
    Write-Host "but Windows BLE API cannot provide the PIN automatically." -ForegroundColor Yellow
    Write-Host "" -ForegroundColor White
    Write-Host "WORKAROUND OPTIONS:" -ForegroundColor Cyan
    Write-Host "" -ForegroundColor White
    Write-Host "Option 1: Disable pairing requirement on device" -ForegroundColor Green
    Write-Host "  - If you have access to MeshCore firmware settings," -ForegroundColor White
    Write-Host "    try disabling BLE security/pairing requirement" -ForegroundColor White
    Write-Host "" -ForegroundColor White
    Write-Host "Option 2: Use a different BLE adapter" -ForegroundColor Green
    Write-Host "  - Some USB BLE adapters handle pairing better" -ForegroundColor White
    Write-Host "" -ForegroundColor White
    Write-Host "Option 3: Connect via Serial/USB instead" -ForegroundColor Green
    Write-Host "  - If your MeshCore device supports USB serial," -ForegroundColor White  
    Write-Host "    that connection doesn't require BLE pairing" -ForegroundColor White
    Write-Host "" -ForegroundColor White
    
    # Clean up
    $bleDevice.Dispose()
}
catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host $_.ScriptStackTrace -ForegroundColor DarkRed
}
