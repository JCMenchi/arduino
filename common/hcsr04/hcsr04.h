/*
    Driver for HC-­SR04 Ultrasonic Sensor

      +-----------------------+
      |   HC-SR04 Module      |
      |   __           __     |
      |  (  )         (  )    |  <-- Ultrasonic Transducers
      |                      |
      |  VCC  TRIG  ECHO  GND |  <-- Pins (bottom view)
      +-----------------------+
         |     |     |     |
        VCC   TRIG  ECHO  GND

  VCC: Power (5V)
  TRIG: Trigger Input
  ECHO: Echo Output
  GND: Ground

  This library provides a simple interface to read distance measurements from the HC-SR04 ultrasonic sensor.
  * The class allows setting the trigger and echo pins, reading distance measurements, and configuring a timeout for readings.
  * The distance is measured in centimeters.
  * The timeout can be adjusted to prevent blocking indefinitely if no echo is received.
  * The default timeout is set to 20,000 microseconds (same as 344 cm).
  * The read method returns the distance in centimeters or 0 if the measurement fails or times out.
  * The sensor operates by sending a short ultrasonic pulse and measuring the time it takes for the echo to return.
  * The sensor should be connected to a microcontroller with the specified trigger and echo pins.
  * The sensor's maximum range is typically around 400 cm, but this can vary based on environmental conditions.
  * The sensor should be used in an environment free from obstacles that could interfere with the ultrasonic waves.
  * The library is designed to be used with AVR microcontroller.
*/

#ifndef HC_SR04_H
#define HC_SR04_H

#include <gpio.h>
#include <stdint.h>

#ifndef HC_SR04_PORTID
#define HC_SR04_PORTID C
#endif

class HC_SR04 {
   public:
    HC_SR04(uint8_t trig, uint8_t echo, uint16_t timeOut = 20000UL)
        : triggerPin(trig), echoPin(echo), timeout(timeOut) {}

    void setup() {
        // Initialize the pins
        GPIO_OUTPUT(HC_SR04_PORTID, triggerPin);
        GPIO_SET_LOW(HC_SR04_PORTID, triggerPin);
        GPIO_INPUT_PULLDOWN(HC_SR04_PORTID, echoPin);
    }

    int16_t read();

    void setTimeout(uint16_t timeOut) { timeout = timeOut; }

   private:
    uint8_t triggerPin;
    uint8_t echoPin;

    uint16_t timeout;
};

#endif  // HC_SR04_H