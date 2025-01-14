#ifndef RAIN_H
#define RAIN_H

#include <Arduino.h>

class Rain
{
public:
    Rain(int sensorPinRain);
    int readSensor();

private:
    int sensorPinRain;
    int numReadings;
    int readings[20];
    int readIndex;
    int total;
    int average;
};

#endif