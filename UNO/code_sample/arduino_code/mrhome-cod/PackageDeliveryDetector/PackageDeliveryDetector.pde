//START:includes
#include <NewSoftSerial.h>
//END:includes

//START:defines
#define FORCE_THRESHOLD 400
#define ONBOARD_LED      13
#define FORCE_SENSOR      1
//END:defines

//START:variables
// Set the Xbee serial transmit/receive digital pins
NewSoftSerial XbeeSerial = NewSoftSerial(2, 3);
int force_value  = 0;
byte force_state = 0;
//END: variables

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

//START:SendDeliveryAlert
void SendDeliveryAlert(int force_value, int force_state)
{
    digitalWrite(ONBOARD_LED, force_state ? HIGH : LOW);
    if (force_state)
        Serial.print("Package delivered, force_value=");
    else
        Serial.print("Package removed, force_value=");
    Serial.println(force_value);
    XbeeSerial.println(force_value);
}
//END:SendDeliveryAlert

//START:main_loop
void loop() 
{
    // wait a second each loop iteration
    delay(1000);

    // poll FLEX_SENSOR voltage
    force_value = analogRead(FORCE_SENSOR);

    switch (force_state)
    {
    case 0: // check if package was delivered
        if (force_value >= FORCE_THRESHOLD)
        {
            force_state = 1;
            SendDeliveryAlert(force_value, force_state);
        }
        break;

    case 1: // check if package was removed
        if (force_value < FORCE_THRESHOLD)
        {
            force_state = 0;
            SendDeliveryAlert(force_value, force_state);
        }
        break;
    }
}
//END:main_loop
