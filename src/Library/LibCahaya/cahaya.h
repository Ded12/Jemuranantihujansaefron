#ifndef CAHAYA_H
#define CAHAYA_H

#include <Arduino.h>

class Cahaya
{
public:
    Cahaya(int sensorPin);
    int readSensor();

private:
    int _sensorPin;
    int _numReadings;
    int _readings[10];
    int _readIndex;
    int _total;
    int _average;
};

#endif