//START:includes
#include <SPI.h>
#include <Ethernet.h>
//END:includes

//START:defines
#define FLEX_TOO_HI   475
#define FLEX_TOO_LOW  465
#define ONBOARD_LED    13
#define FLEX_SENSOR     0
//END:defines

//START:variables
int bend_value  = 0;
byte bend_state = 0;
//END: variables

//START:network_variables
// configure the Ethernet Shield parameters
byte MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };

// replace this shield IP address with one that resides within 
// your own network range
byte IPADDR[]  = { 192, 168, 1, 230 }; 

// replace with your gateway/router address
byte GATEWAY[] = { 192, 168, 1, 1 };

// replace with your subnet address
byte SUBNET[]  = { 255, 255, 255, 0 };

// replace this server IP address with that of your PHP server 
byte PHPSVR[] = {???, ???, ???, ???};

// initialize a Client object and assign it to your PHP server's 
// IP address connecting over the standard HTTP port 80
Client client(PHPSVR, 80);
//END:network_variables

//START:setup
void setup() 
{
    // for serial window debugging
    Serial.begin(9600);

    // set up on board led on digital pin 13 for output
    pinMode(ONBOARD_LED, OUTPUT);

    // Initialize Ethernet Shield with defined MAC and IP address
    Ethernet.begin(MAC, IPADDR, GATEWAY, SUBNET);
    // Wait for Ethernet shield to initialize
    delay(1000);
}
//END:setup

//START:ContactWebServer
void ContactWebServer(int bend_value, int bend_state)
{
    Serial.println("Connecting to the web server to send alert notification...");

    if (client.connect()) 
    {
        Serial.println("Connected to PHP server");
        // Make a HTTP request:
        client.print("GET /wateralert.php?alert=");
        client.print(bend_state);
        client.print("&flex=");
        client.print(bend_value);
        client.println(" HTTP/1.0");
        client.println();
        client.stop();
    } 
    else 
    {
        Serial.println("Failed to connect to the web server");
    }
}
//END:ContactWebServer

//START:SendWaterAlert
void SendWaterAlert(int bend_value, int bend_state)
{
    digitalWrite(ONBOARD_LED, bend_state ? HIGH : LOW);
    if (bend_state)
        Serial.print("Water Level Threshold Exceeded, bend_value=");
    else
        Serial.print("Water Level Returned To Normal bend_value=");
    Serial.println(bend_value);
    ContactWebServer(bend_value, bend_state);
}
//END:SendWaterAlert

//START:main_loop
void loop() 
{
    // wait a second each loop iteration
    delay(1000);

    // poll FLEX_SENSOR voltage
    bend_value = analogRead(FLEX_SENSOR);

	// print bend_value to the serial port for baseline measurement
	// comment this out once baseline, upper and lower threshold
	// limits have been defined
    Serial.print("bend_value=");
    Serial.println(bend_value);

    switch (bend_state)
    {
    case 0: // bend_value does not exceed high or low values
        if (bend_value >= FLEX_TOO_HI || bend_value <= FLEX_TOO_LOW)
        {
            bend_state = 1;
            SendWaterAlert(bend_value, bend_state);
        }
        break;

    case 1: // bend_value exceeds high or low values
        if (bend_value < FLEX_TOO_HI && bend_value > FLEX_TOO_LOW)
        {
            bend_state = 0;
            SendWaterAlert(bend_value, bend_state);
        }
        break;
    }
}
//END:main_loop