#include "CayenneLpp.h"
#include "BufferReader.h"
#include <QVariantMap>

namespace MeshCore {

QList<TelemetryValue> CayenneLpp::parse(const QByteArray &data)
{
    QList<TelemetryValue> telemetry;
    BufferReader reader(data);

    while (reader.remainingBytes() >= 2) {
        quint8 channel = reader.readUInt8();
        quint8 type = reader.readUInt8();

        // Stop if channel and type are zero (garbage bytes)
        if (channel == 0 && type == 0) {
            break;
        }

        try {
            switch (type) {
            case GenericSensor: {
                if (reader.remainingBytes() < 4) return telemetry;
                quint32 value = reader.readUInt32BE();
                telemetry.append(TelemetryValue(channel, type, value));
                break;
            }
            case Luminosity: {
                if (reader.remainingBytes() < 2) return telemetry;
                qint16 lux = reader.readInt16BE();
                telemetry.append(TelemetryValue(channel, type, lux));
                break;
            }
            case Presence: {
                if (reader.remainingBytes() < 1) return telemetry;
                quint8 presence = reader.readUInt8();
                telemetry.append(TelemetryValue(channel, type, presence != 0));
                break;
            }
            case Temperature: {
                if (reader.remainingBytes() < 2) return telemetry;
                double temp = reader.readInt16BE() / 10.0;
                telemetry.append(TelemetryValue(channel, type, temp));
                break;
            }
            case RelativeHumidity: {
                if (reader.remainingBytes() < 1) return telemetry;
                double humidity = reader.readUInt8() / 2.0;
                telemetry.append(TelemetryValue(channel, type, humidity));
                break;
            }
            case BarometricPressure: {
                if (reader.remainingBytes() < 2) return telemetry;
                double pressure = reader.readUInt16BE() / 10.0;
                telemetry.append(TelemetryValue(channel, type, pressure));
                break;
            }
            case Voltage: {
                if (reader.remainingBytes() < 2) return telemetry;
                // Using signed to allow negative voltage
                double voltage = reader.readInt16BE() / 100.0;
                telemetry.append(TelemetryValue(channel, type, voltage));
                break;
            }
            case Current: {
                if (reader.remainingBytes() < 2) return telemetry;
                double current = reader.readInt16BE() / 1000.0;
                telemetry.append(TelemetryValue(channel, type, current));
                break;
            }
            case Percentage: {
                if (reader.remainingBytes() < 1) return telemetry;
                quint8 percent = reader.readUInt8();
                telemetry.append(TelemetryValue(channel, type, percent));
                break;
            }
            case Concentration: {
                if (reader.remainingBytes() < 2) return telemetry;
                quint16 ppm = reader.readUInt16BE();
                telemetry.append(TelemetryValue(channel, type, ppm));
                break;
            }
            case Power: {
                if (reader.remainingBytes() < 2) return telemetry;
                quint16 power = reader.readUInt16BE();
                telemetry.append(TelemetryValue(channel, type, power));
                break;
            }
            case Gps: {
                if (reader.remainingBytes() < 9) return telemetry;
                double lat = reader.readInt24BE() / 10000.0;
                double lon = reader.readInt24BE() / 10000.0;
                double alt = reader.readInt24BE() / 100.0;
                QVariantMap gps;
                gps[QStringLiteral("latitude")] = lat;
                gps[QStringLiteral("longitude")] = lon;
                gps[QStringLiteral("altitude")] = alt;
                telemetry.append(TelemetryValue(channel, type, gps));
                break;
            }
            case DigitalInput:
            case DigitalOutput:
            case Switch: {
                if (reader.remainingBytes() < 1) return telemetry;
                quint8 value = reader.readUInt8();
                telemetry.append(TelemetryValue(channel, type, value != 0));
                break;
            }
            case AnalogInput:
            case AnalogOutput: {
                if (reader.remainingBytes() < 2) return telemetry;
                double value = reader.readInt16BE() / 100.0;
                telemetry.append(TelemetryValue(channel, type, value));
                break;
            }
            case Altitude: {
                if (reader.remainingBytes() < 2) return telemetry;
                qint16 alt = reader.readInt16BE();
                telemetry.append(TelemetryValue(channel, type, alt));
                break;
            }
            case Frequency: {
                if (reader.remainingBytes() < 4) return telemetry;
                quint32 freq = reader.readUInt32BE();
                telemetry.append(TelemetryValue(channel, type, freq));
                break;
            }
            case Distance: {
                if (reader.remainingBytes() < 4) return telemetry;
                double dist = reader.readUInt32BE() / 1000.0;
                telemetry.append(TelemetryValue(channel, type, dist));
                break;
            }
            case Energy: {
                if (reader.remainingBytes() < 4) return telemetry;
                double energy = reader.readUInt32BE() / 1000.0;
                telemetry.append(TelemetryValue(channel, type, energy));
                break;
            }
            case Direction: {
                if (reader.remainingBytes() < 2) return telemetry;
                quint16 dir = reader.readUInt16BE();
                telemetry.append(TelemetryValue(channel, type, dir));
                break;
            }
            case UnixTime: {
                if (reader.remainingBytes() < 4) return telemetry;
                quint32 time = reader.readUInt32BE();
                telemetry.append(TelemetryValue(channel, type, time));
                break;
            }
            default:
                // Unsupported type - can't continue parsing as we don't know the size
                return telemetry;
            }
        } catch (...) {
            // Stop parsing on any error
            break;
        }
    }

    return telemetry;
}

QString CayenneLpp::typeName(quint8 type)
{
    switch (type) {
    case DigitalInput: return QStringLiteral("Digital Input");
    case DigitalOutput: return QStringLiteral("Digital Output");
    case AnalogInput: return QStringLiteral("Analog Input");
    case AnalogOutput: return QStringLiteral("Analog Output");
    case GenericSensor: return QStringLiteral("Generic Sensor");
    case Luminosity: return QStringLiteral("Luminosity");
    case Presence: return QStringLiteral("Presence");
    case Temperature: return QStringLiteral("Temperature");
    case RelativeHumidity: return QStringLiteral("Humidity");
    case Accelerometer: return QStringLiteral("Accelerometer");
    case BarometricPressure: return QStringLiteral("Pressure");
    case Voltage: return QStringLiteral("Voltage");
    case Current: return QStringLiteral("Current");
    case Frequency: return QStringLiteral("Frequency");
    case Percentage: return QStringLiteral("Percentage");
    case Altitude: return QStringLiteral("Altitude");
    case Concentration: return QStringLiteral("Concentration");
    case Power: return QStringLiteral("Power");
    case Distance: return QStringLiteral("Distance");
    case Energy: return QStringLiteral("Energy");
    case Direction: return QStringLiteral("Direction");
    case UnixTime: return QStringLiteral("Unix Time");
    case Gyrometer: return QStringLiteral("Gyrometer");
    case Colour: return QStringLiteral("Colour");
    case Gps: return QStringLiteral("GPS");
    case Switch: return QStringLiteral("Switch");
    default: return QStringLiteral("Unknown");
    }
}

} // namespace MeshCore
