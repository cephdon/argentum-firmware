//
//  Motor.h
//
//
//  Created by Isabella Stephens on 10/12/12.
//  Copyright (c) 2012 Cartesian Co. All rights reserved.
//

#ifndef _Motor_h
#define _Motor_h

#include "Arduino.h"

class Motor {
public:
    //constructor:
    Motor(int stepPin, int dirPin, int powerPin, int stepsPerRev);
    Motor();
    void power(int state);
    long getPosition();
    void resetPosition();
    void switchPower();
    void setDir(int direction);
    void step();
    void setSpeed(int mmPerMinute); //set to 0 for instantaneous movement
    int getSpeed();
    void move(long steps);
    long lastStepTime;


private:
    int stepPin;
    int dirPin;
    int powerPin;
    int stepsPerRev;
    double position;
    int direction;
    int speed;
    //static const long stepsPerMeter = 94140;//11.767463 (half steps per mm)
    static const long stepsPerMeter = 80000; //0.0125 mm per step
};

#endif
