#include "rain.h"

Rain::Rain(int sensorPinRain) : sensorPinRain(sensorPinRain),
                                numReadings(10),
                                readIndex(0),
                                total(0),
                                average(0)
{
    for (int i = 0; i < numReadings; i++)
    {
        readings[i] = 0;
    }
}
int Rain::readSensor()
{
    // Baca nilai sensor
    int sensorValue = analogRead(sensorPinRain);
    sensorValue = max(0, 4095 - sensorValue); // Ubah 1023 menjadi 4095 untuk ESP32

    total -= readings[readIndex];
    readings[readIndex] = sensorValue;
    total += readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;
    std::sort(readings, readings + numReadings);
    average = readings[numReadings / 2];

    return average;
}
// int Cahaya::readSensor() {
//     int sensorValue = analogRead(_sensorPin);

//     _total -= _readings[_readIndex];
//     _readings[_readIndex] = sensorValue;
//     _total += _readings[_readIndex];
//     _readIndex = (_readIndex + 1) % _numReadings;
//     std::sort(_readings, _readings + _numReadings);
//     _average = _readings[_numReadings / 2];

//     return _average;
// }