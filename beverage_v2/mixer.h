#ifndef MIXER_H
#define MIXER_H
#include <Arduino.h>
#include "pump.h"


class Mixer {
    public:
        Mixer();
        String name;
        double capacity;
        double remaining;
        bool isEmpty;
        void dispense(double oz);
        void dispenseShot();
};


#endif