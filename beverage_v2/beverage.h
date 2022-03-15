#ifndef BEVERAGE_H
#define BEVERAGE_H
#include <Arduino.h>
#include "mixer.h"

class Beverage {
    public:
        Beverage(String name, Mixer ms[6]);
        String name;
        Mixer mixers[6];
        
};

#endif