//START:includes
#include <CapSense.h>;
#include <NewSoftSerial.h>
//END:includes

//START:defines
#define ON_PERCH        1500
#define SEED             500
#define CAP_SENSE         30
#define ONBOARD_LED       13
#define PHOTOCELL_SENSOR   0
//END:defines

//START:variables
// Set the Xbee serial transmit/receive digital pins
NewSoftSerial XbeeSerial = NewSoftSerial(2, 3);
CapSense foil_sensor     = CapSense(10,7); // capacitive sensor
                         // resistor bridging digital pins 10 and 7,
                         // wire attached to pin 7 side of resistor
int perch_value  = 0;
byte perch_state = 0;
int seed_value   = 0;
byte seed_state  = 0;
//END:variables

//START:setup
void setup()
{
    // for serial window debugging
    Serial.begin(9600);

    // for Xbee transmission
    XbeeSerial.begin(9600);

    // set pin for onboard led 
    pinMode(ONBOARD_LED, OUTPUT);
}
//END:setup

//START:SendPerchAlert
void SendPerchAlert(int perch_value, int perch_state)
{
    digitalWrite(ONBOARD_LED, perch_state ? HIGH : LOW);
    if (perch_state) 
    {
        XbeeSerial.println("arrived");
        Serial.print("Perch arrival event, perch_value=");
	}
    else
    {
        XbeeSerial.println("departed");  
        Serial.print("Perch departure event, perch_value=");
    }
    Serial.println(perch_value);
}
//END:SendPerchAlert

//START:SendSeedAlert
void SendSeedAlert(int seed_value, int seed_state)
{
    digitalWrite(ONBOARD_LED, seed_state ? HIGH : LOW);
    if (seed_state)
    {
        XbeeSerial.println("refill");
        Serial.print("Refill seed, seed_value=");
    }
    else
    {
        XbeeSerial.println("seedOK");
        Serial.print("Seed refilled, seed_value=");
    }
    Serial.println(seed_value);
}
//END:SendSeedAlert

//START:main_loop
void loop() {
    // wait a second each loop iteration
    delay(1000);

    // poll foil perch value
    perch_value =  foil_sensor.capSense(CAP_SENSE);

    // poll photocell value for seeds
    seed_value =  analogRead(PHOTOCELL_SENSOR);

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

    switch (seed_state)
    {
    case 0: // bird feeder seed filled
        if (seed_value >= SEED)
        {
            seed_state = 1;
            SendSeedAlert(seed_value, seed_state);
        }
        break;

    case 1: // bird feeder seed empty
        if (seed_value < SEED)
        {
            seed_state = 0;
            SendSeedAlert(seed_value, seed_state);
        }
        break;
    }
}
//END:main_loop
