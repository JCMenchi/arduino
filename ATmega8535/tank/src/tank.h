#ifndef TANK_H
#define TANK_H

#include <gpio.h>
#include <pwm.h>
#include <shiftregister.h>
#include <usart_serial.h>
#include <SSD1306Display.h>
#include <int0_serial.h>


#ifndef MOTOR1_DRIVER_ON
#define MOTOR1_DRIVER_ON SR_Q0
#endif

#ifndef MOTOR1_FORWARD
#define MOTOR1_FORWARD SR_Q1
#endif

#ifndef MOTOR1_REVERSE
#define MOTOR1_REVERSE SR_Q2
#endif

#ifndef MOTOR1_ON
#define MOTOR1_ON SR_Q3
#endif

#ifndef MOTOR2_FORWARD
#define MOTOR2_FORWARD SR_Q4
#endif

#ifndef MOTOR2_REVERSE
#define MOTOR2_REVERSE SR_Q5
#endif

#ifndef MOTOR2_ON
#define MOTOR2_ON SR_Q6
#endif

#ifndef MOTOR_SPEED_CONTROL
#define MOTOR_SPEED_CONTROL PWM_OC2
#endif

#ifndef DATA_PIN
#define DATA_PIN 1  // DS
#endif

#ifndef LATCH_PIN
#define LATCH_PIN 2  // ST_CP
#endif

#ifndef CLOCK_PIN
#define CLOCK_PIN 3  // SH_CP
#endif

#define TANK_MOVING_MASK (1 << MOTOR1_ON | 1 << MOTOR2_ON)

const uint8_t minSpeed = 150;
const uint8_t maxSpeed = 255;

class Tank {
public:
    Tank() : m_speed(minSpeed), shiftreg(DATA_PIN, LATCH_PIN, CLOCK_PIN) {}

    inline void info() {
        USART_WriteString("Tank is: ");
        if (shiftreg.getState(MOTOR1_ON) == 1 && shiftreg.getState(MOTOR2_ON) == 1) {
            if (shiftreg.getState(MOTOR1_FORWARD) == 1 && shiftreg.getState(MOTOR2_FORWARD) == 1) {
                USART_WriteString("going forward");
                INT0_WriteChar('F');
            } else if (shiftreg.getState(MOTOR1_REVERSE) == 1 && shiftreg.getState(MOTOR2_REVERSE) == 1) {
                USART_WriteString("going backward");
                INT0_WriteChar('B');
            }
        } else if (shiftreg.getState(MOTOR1_ON) == 1 && shiftreg.getState(MOTOR2_ON) == 0) {
            USART_WriteString(" turning left");
            INT0_WriteChar('L');
        } else if (shiftreg.getState(MOTOR1_ON) == 0 && shiftreg.getState(MOTOR2_ON) == 1) {
            USART_WriteString(" turning right");
            INT0_WriteChar('R');
        } else {
            USART_WriteString("stopped");
            INT0_WriteChar('S');
        }

        USART_WriteString(", speed: ");
        USART_WriteUInt(m_speed, 10);

        INT0_WriteUInt(m_speed, 10);

        USART_WriteString("\n");

        INT0_WriteString("\n");
    }

    void display(SSD1306Display *display) {
        display->drawPage(3, 0x00);
        display->drawString(0,24, "Tank:");
        if (shiftreg.getState(MOTOR1_ON) == 1 && shiftreg.getState(MOTOR2_ON) == 1) {
            if (shiftreg.getState(MOTOR1_FORWARD) == 1 && shiftreg.getState(MOTOR2_FORWARD) == 1) {
                display->drawString(7*5,24,"forward");
            } else if (shiftreg.getState(MOTOR1_REVERSE) == 1 && shiftreg.getState(MOTOR2_REVERSE) == 1) {
                display->drawString(7*5,24,"backward");
            }
        } else if (shiftreg.getState(MOTOR1_ON) == 1 && shiftreg.getState(MOTOR2_ON) == 0) {
            display->drawString(7*5,24,"left");
        } else if (shiftreg.getState(MOTOR1_ON) == 0 && shiftreg.getState(MOTOR2_ON) == 1) {
            display->drawString(7*5,24,"right");
        } else {
            display->drawString(7*5,24,"stopped");
        }

        display->drawString(17*5,24,"sp:");
        display->drawInt(21*5,24,m_speed, 10);
    }

    inline void setup() {
        // initialize
        shiftreg.setup();
        shiftreg.allLow();
        enablePWM(MOTOR_SPEED_CONTROL);  // PORT D 7

        // turn led on to shwo we are ready
        shiftreg.setState(MOTOR1_DRIVER_ON, 1);
        shiftreg.sendData();
    }

    bool isMoving() const { 
        return (shiftreg.getMask() & TANK_MOVING_MASK) != 0;
    }
    
    bool isMovingForward() const { 
        return (shiftreg.getMask() & TANK_MOVING_MASK) != 0 && 
               (shiftreg.getState(MOTOR1_FORWARD) == 1 || shiftreg.getState(MOTOR2_FORWARD) == 1);
    }

    ShiftRegisterPort* shiftregistry() {
        return &shiftreg;
    }

    inline void forward() {
        setPWM(MOTOR_SPEED_CONTROL, 0);

        shiftreg.setState(MOTOR1_FORWARD, 1);
        shiftreg.setState(MOTOR1_REVERSE, 0);
        shiftreg.setState(MOTOR1_ON, 1);
        shiftreg.setState(MOTOR2_FORWARD, 1);
        shiftreg.setState(MOTOR2_REVERSE, 0);
        shiftreg.setState(MOTOR2_ON, 1);

        shiftreg.sendData();

        setPWM(MOTOR_SPEED_CONTROL, m_speed);
    }

    inline void backward() {
        setPWM(MOTOR_SPEED_CONTROL, 0);
        shiftreg.setState(MOTOR1_FORWARD, 0);
        shiftreg.setState(MOTOR1_REVERSE, 1);
        shiftreg.setState(MOTOR1_ON, 1);
        shiftreg.setState(MOTOR2_FORWARD, 0);
        shiftreg.setState(MOTOR2_REVERSE, 1);
        shiftreg.setState(MOTOR2_ON, 1);
        shiftreg.sendData();

        setPWM(MOTOR_SPEED_CONTROL, m_speed);
    }

    inline void left() {
        
        shiftreg.setState(MOTOR1_FORWARD, 1);
        shiftreg.setState(MOTOR1_REVERSE, 0);
        shiftreg.setState(MOTOR1_ON, 1);
        shiftreg.setState(MOTOR2_FORWARD, 0);
        shiftreg.setState(MOTOR2_REVERSE, 1);
        shiftreg.setState(MOTOR2_ON, 0);
        shiftreg.sendData();

        setPWM(MOTOR_SPEED_CONTROL, m_speed);
    }

    inline void right() {
        setPWM(MOTOR_SPEED_CONTROL, 0);
        shiftreg.setState(MOTOR1_FORWARD, 0);
        shiftreg.setState(MOTOR1_REVERSE, 1);
        shiftreg.setState(MOTOR1_ON, 0);
        shiftreg.setState(MOTOR2_FORWARD, 1);
        shiftreg.setState(MOTOR2_REVERSE, 0);
        shiftreg.setState(MOTOR2_ON, 1);
        shiftreg.sendData();

        setPWM(MOTOR_SPEED_CONTROL, m_speed);
    }

    inline void stop() {
        setPWM(MOTOR_SPEED_CONTROL, 0);
        shiftreg.setState(MOTOR1_FORWARD, 0);
        shiftreg.setState(MOTOR1_REVERSE, 0);
        shiftreg.setState(MOTOR1_ON, 0);
        shiftreg.setState(MOTOR2_FORWARD, 0);
        shiftreg.setState(MOTOR2_REVERSE, 0);
        shiftreg.setState(MOTOR2_ON, 0);
        shiftreg.sendData();
    }

    inline void accel() {
        if (m_speed > maxSpeed - 10) {
            m_speed = maxSpeed;
        } else {
            m_speed += 10;
        }
        setPWM(MOTOR_SPEED_CONTROL, m_speed);
    }

    inline void decel() {
        if (m_speed < minSpeed - 10) {
            m_speed = minSpeed;
        } else {
            m_speed -= 10;
        }
        setPWM(MOTOR_SPEED_CONTROL, m_speed);
    }

   private:
    uint8_t m_speed;
    ShiftRegisterPort shiftreg;
};

#endif