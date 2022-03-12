#ifndef BEVERAGE_H
#define BEVERAGE_H
#include <Arduino.h>
#include "mixer.h"

class Beverage {
    public:
        Beverage(String name, Mixer mixers[]);
        String name;
        Mixer mixers[];
};

#endif