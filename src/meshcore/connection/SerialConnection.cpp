#include "SerialConnection.h"
#include "../MeshCoreConstants.h"
#include "../utils/BufferReader.h"
#include "../utils/BufferWriter.h"
#include <QDebug>
#include <QThread>

namespace MeshCore {

SerialConnection::SerialConnection(QObject *parent)
    : MeshCoreConnection(parent)
{
}

SerialConnection::~SerialConnection()
{
    close();
}

void SerialConnection::connectToPort(const QString &portName, qint32 baudRate)
{
    connectToPort(QSerialPortInfo(portName), baudRate);
}

void SerialConnection::connectToPort(const QSerialPortInfo &portInfo, qint32 baudRate)
{
    if (m_serialPort) {
        close();
    }

    m_serialPort = std::make_unique<QSerialPort>(portInfo, this);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_serialPort.get(), &QSerialPort::readyRead,
            this, &SerialConnection::onReadyRead);
    connect(m_serialPort.get(), &QSerialPort::errorOccurred,
            this, &SerialConnection::onErrorOccurred);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        Q_EMIT errorOccurred(QStringLiteral("Failed to open serial port: %1")
                             .arg(m_serialPort->errorString()));
        m_serialPort.reset();
        return;
    }

    // Set RTS to false (required by MeshCore protocol, as per Python implementation)
    m_serialPort->setRequestToSend(false);
    
    // Brief wait for device to be ready
    QThread::msleep(50);

    // Clear any pending data
    m_serialPort->clear();
    m_readBuffer.clear();

    qDebug() << "Serial connected to" << portInfo.portName();
    onConnected();
}

void SerialConnection::close()
{
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        m_serialPort.reset();
    }
    m_readBuffer.clear();
    onDisconnected();
}

QString SerialConnection::portName() const
{
    return m_serialPort ? m_serialPort->portName() : QString();
}

void SerialConnection::onReadyRead()
{
    if (!m_serialPort) {
        return;
    }

    QByteArray newData = m_serialPort->readAll();
    m_readBuffer.append(newData);
    processReadBuffer();
}

void SerialConnection::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    QString errorStr;
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        errorStr = QStringLiteral("Device not found");
        break;
    case QSerialPort::PermissionError:
        errorStr = QStringLiteral("Permission denied");
        break;
    case QSerialPort::OpenError:
        errorStr = QStringLiteral("Could not open device");
        break;
    case QSerialPort::WriteError:
        errorStr = QStringLiteral("Write error");
        break;
    case QSerialPort::ReadError:
        errorStr = QStringLiteral("Read error");
        break;
    case QSerialPort::ResourceError:
        errorStr = QStringLiteral("Resource error (device disconnected?)");
        close();
        break;
    case QSerialPort::TimeoutError:
        errorStr = QStringLiteral("Timeout");
        break;
    default:
        errorStr = QStringLiteral("Unknown error");
        break;
    }

    Q_EMIT errorOccurred(errorStr);
}

void SerialConnection::processReadBuffer()
{
    // Frame format: [type:1][length:2LE][data:length]
    constexpr qsizetype frameHeaderLength = 3;

    while (m_readBuffer.size() >= frameHeaderLength) {
        // Peek at frame header
        quint8 frameType = static_cast<quint8>(m_readBuffer.at(0));

        // Validate frame type - must be '>' (0x3e) for incoming or '<' (0x3c) for outgoing
        if (frameType != SerialFrameTypes::Incoming && frameType != SerialFrameTypes::Outgoing) {
            // Not a valid frame start - could be debug text from device
            // Skip until newline or valid frame marker
            int newlinePos = m_readBuffer.indexOf('\n');
            int frameMarkerPos = m_readBuffer.indexOf(static_cast<char>(SerialFrameTypes::Incoming));
            
            if (frameMarkerPos > 0 && (newlinePos < 0 || frameMarkerPos < newlinePos)) {
                // Found a frame marker before newline, skip to it
                QByteArray skipped = m_readBuffer.left(frameMarkerPos);
                qDebug() << "Skipping non-frame data:" << skipped;
                m_readBuffer.remove(0, frameMarkerPos);
            } else if (newlinePos >= 0) {
                // Skip the debug line including newline
                QByteArray debugLine = m_readBuffer.left(newlinePos + 1);
                // Only log if it looks like debug output
                if (debugLine.contains("DEBUG") || debugLine.contains("INFO") || debugLine.contains("WARN")) {
                    qDebug() << "Device debug:" << debugLine.trimmed();
                }
                m_readBuffer.remove(0, newlinePos + 1);
            } else {
                // No newline yet, might be partial debug line - wait for more data
                // But limit buffer size to prevent infinite growth
                if (m_readBuffer.size() > 1024) {
                    qDebug() << "Discarding oversized buffer without frame marker";
                    m_readBuffer.clear();
                }
                break;
            }
            continue;
        }

        // Read frame length (little endian)
        quint16 frameLength = static_cast<quint8>(m_readBuffer.at(1))
                            | (static_cast<quint8>(m_readBuffer.at(2)) << 8);

        if (frameLength == 0 || frameLength > 1024) {
            // Invalid length, skip this byte
            qDebug() << "Invalid frame length:" << frameLength << ", skipping byte";
            m_readBuffer.remove(0, 1);
            continue;
        }

        // Check if we have the complete frame
        qsizetype totalLength = frameHeaderLength + frameLength;
        if (m_readBuffer.size() < totalLength) {
            // Wait for more data
            break;
        }

        // Extract frame data
        QByteArray frameData = m_readBuffer.mid(frameHeaderLength, frameLength);
        m_readBuffer.remove(0, totalLength);

        qDebug() << "Received frame type:" << Qt::hex << frameType 
                 << "length:" << frameLength << "data:" << frameData.toHex();

        // Process the frame (only process incoming frames from device)
        if (frameType == SerialFrameTypes::Incoming) {
            onFrameReceived(frameData);
        }
    }
}

void SerialConnection::sendToRadioFrame(const QByteArray &frame)
{
    qDebug() << "Serial sending frame:" << frame.size() << "bytes, data:" << frame.toHex();
    Q_EMIT frameSent(frame);
    // Send as "app to radio" frame (0x3c = '<')
    writeFrame(SerialFrameTypes::Outgoing, frame);
}

void SerialConnection::writeFrame(quint8 frameType, const QByteArray &frameData)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        qWarning() << "Cannot write: serial port not open";
        return;
    }

    // Build frame: [type:1][length:2LE][data]
    BufferWriter writer;
    writer.writeByte(frameType);
    writer.writeUInt16LE(static_cast<quint16>(frameData.size()));
    writer.writeBytes(frameData);

    QByteArray fullFrame = writer.toByteArray();
    qDebug() << "Serial RAW TX:" << fullFrame.toHex(' ');
    
    qint64 written = m_serialPort->write(fullFrame);
    m_serialPort->flush();  // Ensure data is sent immediately
    qDebug() << "Serial wrote" << written << "bytes";
}

} // namespace MeshCore
