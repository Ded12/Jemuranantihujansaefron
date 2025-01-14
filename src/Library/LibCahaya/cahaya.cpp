#include "cahaya.h"

Cahaya::Cahaya(int sensorPin) : _sensorPin(sensorPin),
                                _numReadings(10),
                                _readIndex(0),
                                _total(0),
                                _average(0)
{
    for (int i = 0; i < _numReadings; i++)
    {
        _readings[i] = 0;
    }
}
int Cahaya::readSensor()
{
    int sensorValue = analogRead(_sensorPin);
    sensorValue = max(0, 1023 - sensorValue);
    _total -= _readings[_readIndex];
    _readings[_readIndex] = sensorValue;
    _total += _readings[_readIndex];
    _readIndex = (_readIndex + 1) % _numReadings;
    std::sort(_readings, _readings + _numReadings);
    _average = _readings[_numReadings / 2];

    return _average;
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