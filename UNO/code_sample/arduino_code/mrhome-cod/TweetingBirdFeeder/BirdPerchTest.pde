//START:includes
#include <CapSense.h>
//END:includes

//START:defines
#define ON_PERCH  1500
#define CAP_SENSE   30
#define ONBOARD_LED 13
//END:defines

//START:variables
CapSense foil_sensor   = CapSense(10,7); // capacitive sensor
                       // resistor bridging digital pins 10 and 7,
                       // wire attached to pin 7 side of resistor
int perch_value  = 0;
byte perch_state = 0;
//END:variables

//START:setup
void setup()
{
    // for serial window debugging
    Serial.begin(9600);

    // set pin for onboard led 
    pinMode(ONBOARD_LED, OUTPUT);
}
//END:setup

//START:SendPerchAlert
void SendPerchAlert(int perch_value, int perch_state)
{
    digitalWrite(ONBOARD_LED, perch_state ? HIGH : LOW);
    if (perch_state)
        Serial.print("Perch arrival event, perch_value=");
    else
        Serial.print("Perch departure event, perch_value=");
    Serial.println(perch_value);
}
//END:SendPerchAlert

//START:main_loop
void loop() {
    // wait a second each loop iteration
    delay(1000);

    // poll foil perch value
    perch_value =  foil_sensor.capSense(CAP_SENSE);

    switch (perch_state)
    {
    case 0: // no bird currently on the perch
        if (perch_value >= ON_PERCH)
        {
            perch_state = 1;
            SendPerchAlert(perch_value, perch_state);
        }
        break;

    case 1: // bird currently on the perch
        if (perch_value < ON_PERCH)
        {
            perch_state = 0;
            SendPerchAlert(perch_value, perch_state);
        }
        break;
    }
}
//END:main_loop