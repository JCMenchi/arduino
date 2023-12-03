#include <inttypes.h>

class Bounce
{

public:
  // Create an instance of the bounce library
  Bounce(); 
  // Attach to a pin (and also sets initial state)
  void attach(int pin);
	// Sets the debounce interval
  void interval(unsigned long interval_millis); 
	// Updates the pin
	// Returns 1 if the state changed
	// Returns 0 if the state did not change
  bool update(); 
	// Returns the updated pin state
  uint8_t read();

  
protected:
  int debounce();
  unsigned long  previous_millis, interval_millis;
  uint8_t debouncedState;
  uint8_t unstableState;
  uint8_t pin;
  uint8_t stateChanged;
};


#define BUTTON_PIN 12
#define LED_PIN 6



// Instantiate a Bounce object
Bounce debouncer = Bounce(); 

void setup() {
  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
  
  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);
  
  //Setup the LED
  pinMode(LED_PIN,OUTPUT);
  
}

void loop() {
 // Update the debouncer
  debouncer.update();
 
 // Get the update value
 int value = debouncer.read();
 
 // Turn on or off the LED
 if ( value == HIGH) {
   digitalWrite(LED_PIN, HIGH );
 } else {
    digitalWrite(LED_PIN, LOW );
 }
 
}


Bounce::Bounce() {
	this->interval_millis = 10;
	
}

void Bounce::attach(int pin) {
 this->pin = pin;
 debouncedState = unstableState = digitalRead(pin);
 #ifdef BOUNCE_LOCK-OUT
 previous_millis = 0;
 #else
 previous_millis = millis();
 #endif
}



void Bounce::interval(unsigned long interval_millis)
{
  this->interval_millis = interval_millis;
  
}


bool Bounce::update()
{

#ifdef BOUNCE_LOCK-OUT
    stateChanged = false;
	// Ignore everything if we are locked out
	if (millis() - previous_millis >= interval_millis) {
		uint8_t currentState = digitalRead(pin);
		if (debouncedState != currentState ) {
			previous_millis = millis();
			debouncedState = currentState;
			stateChanged = true;
		}
	}
	return stateChanged;

#else
	// Lire l'etat de l'interrupteur dans une variable temporaire.
	uint8_t currentState = digitalRead(pin);
	stateChanged = false;

	// Redemarrer le compteur timeStamp tant et aussi longtemps que
	// la lecture ne se stabilise pas.
	if ( currentState != unstableState ) {
			previous_millis = millis();
	} else 	if ( millis() - previous_millis >= interval_millis ) {
				// Rendu ici, la lecture est stable

				// Est-ce que la lecture est diffÃ©rente de l'etat emmagasine de l'interrupteur?
				if ( currentState != debouncedState ) {
						debouncedState = currentState;
						stateChanged = true;
						
				}

	}
	 
	unstableState = currentState;
	return stateChanged;
#endif

}

uint8_t Bounce::read()
{
	return debouncedState;
}
