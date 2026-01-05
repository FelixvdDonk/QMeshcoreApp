#ifndef BUFFERWRITER_H
#define BUFFERWRITER_H

#include <QByteArray>
#include <QString>
#include <cstdint>

namespace MeshCore {

/**
 * @brief Utility class for writing binary data to a byte buffer
 *
 * Provides methods to write various data types to a QByteArray buffer.
 * Data is appended sequentially.
 */
class BufferWriter
{
public:
    BufferWriter() = default;
    explicit BufferWriter(qsizetype reserveSize);

    // Get the accumulated data
    [[nodiscard]] QByteArray toByteArray() const { return m_buffer; }
    [[nodiscard]] const QByteArray &data() const { return m_buffer; }
    [[nodiscard]] qsizetype size() const { return m_buffer.size(); }

    // Clear the buffer
    void clear() { m_buffer.clear(); }

    // Single byte writes
    void writeByte(quint8 value);
    void writeInt8(qint8 value) { writeByte(static_cast<quint8>(value)); }
    void writeUInt8(quint8 value) { writeByte(value); }

    // Multi-byte writes (Little Endian)
    void writeInt16LE(qint16 value);
    void writeUInt16LE(quint16 value);
    void writeInt32LE(qint32 value);
    void writeUInt32LE(quint32 value);

    // Multi-byte writes (Big Endian)
    void writeInt16BE(qint16 value);
    void writeUInt16BE(quint16 value);
    void writeInt32BE(qint32 value);
    void writeUInt32BE(quint32 value);

    // Byte array writes
    void writeBytes(const QByteArray &data);
    void writeBytes(const char *data, qsizetype size);

    // String writes
    void writeString(const QString &str);                       // Write as UTF-8 bytes
    void writeCString(const QString &str, qsizetype maxLength); // Write null-terminated with fixed length

private:
    QByteArray m_buffer;
};

} // namespace MeshCore

#endif // BUFFERWRITER_H
