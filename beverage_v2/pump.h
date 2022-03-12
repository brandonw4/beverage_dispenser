#ifndef PUMP_H
#define PUMP_H
#include <Arduino.h>

class Pump {
    public:
        bool status;
        void control();
        Pump();
};


#endif