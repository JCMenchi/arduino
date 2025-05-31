#include "hcsr04.h"

#include <util/delay.h>
#include <millisec.h>

#include <usart_serial.h>

#define MICROSEC_ROUNDTRIP_PER_CM 58 // number of ms for 1 cm (sound speed is approximately 343 m/s)
                                     // if obstacle distance is 1 cm, sound travels 2 cm (to the obstacle and back)

#define TRIGGER_DELAY_US 12 // Trigger pulse duration in microseconds
#define MAX_SENSOR_DELAY_MS 6 // Maximum ms it takes for sensor to start the ping. Default=5800 us
#define STEP_RESOLUTION 10 // in microsecond
#define STEP_CORRECTION 5 // in microsecond

int16_t HC_SR04::read() {

    //USART_WriteString("HC-SR04 Trig high\n");
    // Send a 10us pulse to trigger the sensor
    GPIO_SET_HIGH(HC_SR04_PORTID, triggerPin);
    _delay_us(TRIGGER_DELAY_US);
    GPIO_SET_LOW(HC_SR04_PORTID, triggerPin);
    //USART_WriteString("HC-SR04 Trig low\n");

    // Wait for echo to start, with timeout
    uint32_t prevTimeMS = milliseconds();
    uint32_t delay = milliseconds() - prevTimeMS;

    while (GPIO_IS_LOW(HC_SR04_PORTID, echoPin) && (delay < MAX_SENSOR_DELAY_MS)) {
        delay = milliseconds() - prevTimeMS;
    }

    if (delay > MAX_SENSOR_DELAY_MS) {
        return -1;
    }
    //USART_WriteString("HC-SR04 Echo high\n");

    // Measure the duration of the echo pulse, in 10 us steps
    uint16_t maxStep = timeout / STEP_RESOLUTION;
    uint16_t step = 0;
    while (GPIO_IS_HIGH(HC_SR04_PORTID, echoPin) && (step < maxStep)) {
        _delay_us(STEP_RESOLUTION); // Wait for a short time to avoid busy waiting
        step++;
    }
    //USART_WriteString("HC-SR04 Echo low\n");
    if (step >= maxStep) {
        return -2;
    }

    // Calculate distance in cm
    return step * (STEP_RESOLUTION + STEP_CORRECTION) / MICROSEC_ROUNDTRIP_PER_CM; // Convert to cm
}