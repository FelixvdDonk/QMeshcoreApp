#ifndef CAYENNELPP_H
#define CAYENNELPP_H

#include <QByteArray>
#include <QList>
#include <QString>
#include "../types/TelemetryData.h"

namespace MeshCore {

/**
 * @brief Cayenne Low Power Payload (LPP) parser
 *
 * Parses telemetry data in CayenneLPP format commonly used by IoT sensors.
 */
class CayenneLpp
{
public:
    // LPP data types
    enum Type : quint8 {
        DigitalInput = 0,
        DigitalOutput = 1,
        AnalogInput = 2,
        AnalogOutput = 3,
        GenericSensor = 100,
        Luminosity = 101,
        Presence = 102,
        Temperature = 103,
        RelativeHumidity = 104,
        Accelerometer = 113,
        BarometricPressure = 115,
        Voltage = 116,
        Current = 117,
        Frequency = 118,
        Percentage = 120,
        Altitude = 121,
        Concentration = 125,
        Power = 128,
        Distance = 130,
        Energy = 131,
        Direction = 132,
        UnixTime = 133,
        Gyrometer = 134,
        Colour = 135,
        Gps = 136,
        Switch = 142,
        Polyline = 240
    };

    /**
     * @brief Parse CayenneLPP formatted bytes into telemetry values
     */
    static QList<TelemetryValue> parse(const QByteArray &data);

    /**
     * @brief Get human-readable name for LPP type
     */
    static QString typeName(quint8 type);

private:
    CayenneLpp() = default;
};

} // namespace MeshCore

#endif // CAYENNELPP_H
