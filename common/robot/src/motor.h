#ifndef _ROBOT_MOTOR_H
#define _ROBOT_MOTOR_H

#include "gpio.h"
#include "pwm.h"

class MotorPortA {

public:

    MotorPortA() 
    : pwm_pin(0), hbridge_pin_fwd(0), hbridge_pin_rev(0), current_speed(0)
    {}

    MotorPortA(uint8_t pwm, uint8_t hbridge_fwd, uint8_t hbridge_rev) 
    : pwm_pin(pwm), hbridge_pin_fwd(hbridge_fwd), hbridge_pin_rev(hbridge_rev), current_speed(0)
    {}

    void setup(uint8_t pwm, uint8_t hbridge_fwd, uint8_t hbridge_rev) {
        pwm_pin = pwm;
        hbridge_pin_fwd = hbridge_fwd;
        hbridge_pin_rev = hbridge_rev;
        current_speed = 0;
    }

    void init() {
        // setup PWM
        enablePWM(pwm_pin);
        setPWM(pwm_pin, 0);

        // setup control pin
        GPIO_OUTPUT(A, hbridge_pin_fwd);
        GPIO_OUTPUT(A, hbridge_pin_rev);

        // init stop
        stop();
    }

    uint8_t speed() const {
        return current_speed;
    }

    void setSpeed(uint8_t s)  {
        current_speed = s;
        if (current_speed > 0) {
            setPWM(pwm_pin, current_speed);
        } else {
            stop();
        }
    }

    void stop() {
        current_speed = 0;
        setPWM(pwm_pin, 0);
        GPIO_SET_LOW(A, hbridge_pin_fwd);
        GPIO_SET_LOW(A, hbridge_pin_rev);
    }

    void forward(uint8_t s) {
        stop();
        current_speed = s;
        if (current_speed > 0) {
            GPIO_SET_HIGH(A, hbridge_pin_fwd);
            setPWM(pwm_pin, current_speed);
        }
    }

    void reverse(uint8_t s) {
        stop();
        current_speed = s;
        if (current_speed > 0) {
            GPIO_SET_HIGH(A, hbridge_pin_rev);
            setPWM(pwm_pin, current_speed);
        }
    }

private:
    uint8_t pwm_pin;
    uint8_t hbridge_pin_fwd;
    uint8_t hbridge_pin_rev;
    uint8_t current_speed;
};

#endif
