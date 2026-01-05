#include "BufferWriter.h"
#include <QtEndian>

namespace MeshCore {

BufferWriter::BufferWriter(qsizetype reserveSize)
{
    m_buffer.reserve(reserveSize);
}

void BufferWriter::writeByte(quint8 value)
{
    m_buffer.append(static_cast<char>(value));
}

void BufferWriter::writeInt16LE(qint16 value)
{
    char bytes[2];
    qToLittleEndian(value, bytes);
    m_buffer.append(bytes, 2);
}

void BufferWriter::writeUInt16LE(quint16 value)
{
    char bytes[2];
    qToLittleEndian(value, bytes);
    m_buffer.append(bytes, 2);
}

void BufferWriter::writeInt32LE(qint32 value)
{
    char bytes[4];
    qToLittleEndian(value, bytes);
    m_buffer.append(bytes, 4);
}

void BufferWriter::writeUInt32LE(quint32 value)
{
    char bytes[4];
    qToLittleEndian(value, bytes);
    m_buffer.append(bytes, 4);
}

void BufferWriter::writeInt16BE(qint16 value)
{
    char bytes[2];
    qToBigEndian(value, bytes);
    m_buffer.append(bytes, 2);
}

void BufferWriter::writeUInt16BE(quint16 value)
{
    char bytes[2];
    qToBigEndian(value, bytes);
    m_buffer.append(bytes, 2);
}

void BufferWriter::writeInt32BE(qint32 value)
{
    char bytes[4];
    qToBigEndian(value, bytes);
    m_buffer.append(bytes, 4);
}

void BufferWriter::writeUInt32BE(quint32 value)
{
    char bytes[4];
    qToBigEndian(value, bytes);
    m_buffer.append(bytes, 4);
}

void BufferWriter::writeBytes(const QByteArray &data)
{
    m_buffer.append(data);
}

void BufferWriter::writeBytes(const char *data, qsizetype size)
{
    m_buffer.append(data, size);
}

void BufferWriter::writeString(const QString &str)
{
    m_buffer.append(str.toUtf8());
}

void BufferWriter::writeCString(const QString &str, qsizetype maxLength)
{
    QByteArray utf8 = str.toUtf8();

    // Create fixed-size buffer
    QByteArray buffer(maxLength, '\0');

    // Copy string bytes (leaving room for null terminator)
    qsizetype copyLen = qMin(utf8.size(), maxLength - 1);
    for (qsizetype i = 0; i < copyLen; ++i) {
        buffer[i] = utf8.at(i);
    }

    // Last byte is always null (already set)
    m_buffer.append(buffer);
}

} // namespace MeshCore
