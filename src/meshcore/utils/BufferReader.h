#ifndef BUFFERREADER_H
#define BUFFERREADER_H

#include <QByteArray>
#include <QString>
#include <cstdint>

namespace MeshCore {

/**
 * @brief Utility class for reading binary data from a byte buffer
 *
 * Provides methods to read various data types from a QByteArray buffer,
 * maintaining an internal read position. All reads advance the position.
 */
class BufferReader
{
public:
    explicit BufferReader(const QByteArray &data);
    explicit BufferReader(const char *data, qsizetype size);

    // Position management
    [[nodiscard]] qsizetype position() const { return m_position; }
    [[nodiscard]] qsizetype remainingBytes() const { return m_data.size() - m_position; }
    [[nodiscard]] bool hasRemaining() const { return remainingBytes() > 0; }
    void skip(qsizetype count);
    void reset() { m_position = 0; }

    // Single byte reads
    [[nodiscard]] quint8 readByte();
    [[nodiscard]] qint8 readInt8();
    [[nodiscard]] quint8 readUInt8() { return readByte(); }

    // Multi-byte reads (Little Endian)
    [[nodiscard]] qint16 readInt16LE();
    [[nodiscard]] quint16 readUInt16LE();
    [[nodiscard]] qint32 readInt32LE();
    [[nodiscard]] quint32 readUInt32LE();

    // Multi-byte reads (Big Endian)
    [[nodiscard]] qint16 readInt16BE();
    [[nodiscard]] quint16 readUInt16BE();
    [[nodiscard]] qint32 readInt32BE();
    [[nodiscard]] quint32 readUInt32BE();

    // Special reads
    [[nodiscard]] qint32 readInt24BE(); // 24-bit signed big-endian

    // Byte array reads
    [[nodiscard]] QByteArray readBytes(qsizetype count);
    [[nodiscard]] QByteArray readRemainingBytes();

    // String reads
    [[nodiscard]] QString readString();           // Reads remaining bytes as UTF-8
    [[nodiscard]] QString readCString(qsizetype maxLength); // Reads null-terminated string

    // Raw data access
    [[nodiscard]] const QByteArray &data() const { return m_data; }

private:
    QByteArray m_data;
    qsizetype m_position = 0;
};

} // namespace MeshCore

#endif // BUFFERREADER_H
