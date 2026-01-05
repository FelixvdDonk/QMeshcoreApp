#include "BufferReader.h"
#include <QtEndian>
#include <stdexcept>

namespace MeshCore {

BufferReader::BufferReader(const QByteArray &data)
    : m_data(data)
{
}

BufferReader::BufferReader(const char *data, qsizetype size)
    : m_data(data, size)
{
}

void BufferReader::skip(qsizetype count)
{
    if (m_position + count > m_data.size()) {
        throw std::out_of_range("BufferReader::skip: not enough bytes");
    }
    m_position += count;
}

quint8 BufferReader::readByte()
{
    if (m_position >= m_data.size()) {
        throw std::out_of_range("BufferReader::readByte: end of buffer");
    }
    return static_cast<quint8>(m_data.at(m_position++));
}

qint8 BufferReader::readInt8()
{
    return static_cast<qint8>(readByte());
}

qint16 BufferReader::readInt16LE()
{
    if (remainingBytes() < 2) {
        throw std::out_of_range("BufferReader::readInt16LE: not enough bytes");
    }
    qint16 value = qFromLittleEndian<qint16>(m_data.constData() + m_position);
    m_position += 2;
    return value;
}

quint16 BufferReader::readUInt16LE()
{
    if (remainingBytes() < 2) {
        throw std::out_of_range("BufferReader::readUInt16LE: not enough bytes");
    }
    quint16 value = qFromLittleEndian<quint16>(m_data.constData() + m_position);
    m_position += 2;
    return value;
}

qint32 BufferReader::readInt32LE()
{
    if (remainingBytes() < 4) {
        throw std::out_of_range("BufferReader::readInt32LE: not enough bytes");
    }
    qint32 value = qFromLittleEndian<qint32>(m_data.constData() + m_position);
    m_position += 4;
    return value;
}

quint32 BufferReader::readUInt32LE()
{
    if (remainingBytes() < 4) {
        throw std::out_of_range("BufferReader::readUInt32LE: not enough bytes");
    }
    quint32 value = qFromLittleEndian<quint32>(m_data.constData() + m_position);
    m_position += 4;
    return value;
}

qint16 BufferReader::readInt16BE()
{
    if (remainingBytes() < 2) {
        throw std::out_of_range("BufferReader::readInt16BE: not enough bytes");
    }
    qint16 value = qFromBigEndian<qint16>(m_data.constData() + m_position);
    m_position += 2;
    return value;
}

quint16 BufferReader::readUInt16BE()
{
    if (remainingBytes() < 2) {
        throw std::out_of_range("BufferReader::readUInt16BE: not enough bytes");
    }
    quint16 value = qFromBigEndian<quint16>(m_data.constData() + m_position);
    m_position += 2;
    return value;
}

qint32 BufferReader::readInt32BE()
{
    if (remainingBytes() < 4) {
        throw std::out_of_range("BufferReader::readInt32BE: not enough bytes");
    }
    qint32 value = qFromBigEndian<qint32>(m_data.constData() + m_position);
    m_position += 4;
    return value;
}

quint32 BufferReader::readUInt32BE()
{
    if (remainingBytes() < 4) {
        throw std::out_of_range("BufferReader::readUInt32BE: not enough bytes");
    }
    quint32 value = qFromBigEndian<quint32>(m_data.constData() + m_position);
    m_position += 4;
    return value;
}

qint32 BufferReader::readInt24BE()
{
    if (remainingBytes() < 3) {
        throw std::out_of_range("BufferReader::readInt24BE: not enough bytes");
    }

    // Read 24-bit big-endian value
    qint32 value = (static_cast<quint8>(m_data.at(m_position)) << 16)
                 | (static_cast<quint8>(m_data.at(m_position + 1)) << 8)
                 | static_cast<quint8>(m_data.at(m_position + 2));
    m_position += 3;

    // Sign extend from 24-bit to 32-bit
    // 0x800000 is the sign bit for 24-bit
    if (value & 0x800000) {
        value -= 0x1000000; // Subtract 2^24 to get correct negative value
    }

    return value;
}

QByteArray BufferReader::readBytes(qsizetype count)
{
    if (remainingBytes() < count) {
        throw std::out_of_range("BufferReader::readBytes: not enough bytes");
    }
    QByteArray result = m_data.mid(m_position, count);
    m_position += count;
    return result;
}

QByteArray BufferReader::readRemainingBytes()
{
    return readBytes(remainingBytes());
}

QString BufferReader::readString()
{
    return QString::fromUtf8(readRemainingBytes());
}

QString BufferReader::readCString(qsizetype maxLength)
{
    QByteArray bytes = readBytes(maxLength);

    // Find null terminator
    qsizetype nullPos = bytes.indexOf('\0');
    if (nullPos >= 0) {
        bytes.truncate(nullPos);
    }

    return QString::fromUtf8(bytes);
}

} // namespace MeshCore
