//START:defines
#define SEED             500
#define ONBOARD_LED       13
#define PHOTOCELL_SENSOR   0
//END:defines

//START:variables
int seed_value  = 0;
byte seed_state = 0;
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

//START:SendSeedAlert
void SendSeedAlert(int seed_value, int seed_state)
{
    digitalWrite(ONBOARD_LED, seed_state ? HIGH : LOW);
    if (seed_state)
        Serial.print("Refill seed, seed_value=");
    else
        Serial.print("Seed refilled, seed_value=");
    Serial.println(seed_value);
}
//END:SendSeedAlert

//START:main_loop
void loop() {
    // wait a second each loop iteration
    delay(1000);

    // poll photocell value for seeds
    seed_value = analogRead(PHOTOCELL_SENSOR);

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