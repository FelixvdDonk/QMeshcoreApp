#ifndef SERIALCONNECTION_H
#define SERIALCONNECTION_H

#include "MeshCoreConnection.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#include <QMutex>

namespace MeshCore {

/**
 * @brief Serial (USB) connection to a MeshCore device
 *
 * Handles framing for serial protocol (adds frame header with type and length).
 */
class SerialConnection : public MeshCoreConnection
{
    Q_OBJECT

public:
    explicit SerialConnection(QObject *parent = nullptr);
    ~SerialConnection() override;

    /**
     * @brief Connect to a serial port
     * @param portName The name of the serial port (e.g., "/dev/ttyUSB0" or "COM3")
     * @param baudRate The baud rate (default 115200)
     */
    void connectToPort(const QString &portName, qint32 baudRate = 115200);

    /**
     * @brief Connect to a serial port
     * @param portInfo The serial port info
     * @param baudRate The baud rate (default 115200)
     */
    void connectToPort(const QSerialPortInfo &portInfo, qint32 baudRate = 115200);

    void close() override;

    [[nodiscard]] QString portName() const;

protected:
    void sendToRadioFrame(const QByteArray &frame) override;

private Q_SLOTS:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    void writeFrame(quint8 frameType, const QByteArray &frameData);
    void processReadBuffer();

    std::unique_ptr<QSerialPort> m_serialPort;
    QByteArray m_readBuffer;
};

} // namespace MeshCore

#endif // SERIALCONNECTION_H
