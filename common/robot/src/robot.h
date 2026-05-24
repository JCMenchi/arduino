#ifndef _ROBOT_ROBOT_H
#define _ROBOT_ROBOT_H

#include "motor.h"


const uint8_t ROBOT_STILL = 0;
const uint8_t ROBOT_GOING_UP = 1;
const uint8_t ROBOT_GOING_DOWN = 2;

const uint8_t ROBOT_UP_PIN = 6;
const uint8_t ROBOT_DOWN_PIN = 7;

#define ROBOT_LED_PORT C
const uint8_t ROBOT_LED_PIN = 7;

class Robot {

public:

    Robot() : bodyState(ROBOT_STILL)
    {}

    void init() {
        GPIO_OUTPUT(ROBOT_LED_PORT, ROBOT_LED_PIN);
        GPIO_SET_HIGH(ROBOT_LED_PORT, ROBOT_LED_PIN);

        GPIO_INPUT(A, ROBOT_UP_PIN);
        GPIO_INPUT_PULLUP(A, ROBOT_UP_PIN);
        GPIO_INPUT(A, ROBOT_DOWN_PIN);
        GPIO_INPUT_PULLUP(A, ROBOT_DOWN_PIN);
    }

    void update() {

    }

    void up() {
        bodyState = ROBOT_GOING_UP;
        GPIO_SET_HIGH(ROBOT_LED_PORT, ROBOT_LED_PIN);
        upMotor.reverse(255);
    }

    void down() {
        bodyState = ROBOT_GOING_DOWN;
        GPIO_SET_HIGH(ROBOT_LED_PORT, ROBOT_LED_PIN);
        upMotor.forward(255);
    }

private:
    MotorPortA leftMotor;
    MotorPortA rightMotor;

    MotorPortA upMotor;
    uint8_t bodyState;
    
};

#endif
