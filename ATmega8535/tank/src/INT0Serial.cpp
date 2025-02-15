
//
// Includes
//
#include <INT0Serial.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <gpio.h>
#include <stddef.h>

#define INT0_PORT D
#define INT0_PIN PIND2

//
// Statics
//
char INT0Serial::_receive_buffer[_SS_MAX_RX_BUFF];
volatile uint8_t INT0Serial::_receive_buffer_tail = 0;
volatile uint8_t INT0Serial::_receive_buffer_head = 0;
INT0Serial* active_object = NULL;
bool INT0Serial::_buffer_overflow = false;
//
// Private methods
//

// This function sets the current object as the "listening"
// one and returns true if it replaces another
bool INT0Serial::listen() {
    if (!_rx_delay_stopbit)
        return false;

    _buffer_overflow = false;
    _receive_buffer_head = _receive_buffer_tail = 0;

    setRxIntMsk(true);
    return true;
}

// Stop listening. Returns true if we were actually listening.
bool INT0Serial::stopListening() {
    setRxIntMsk(false);
    return true;
}

//
// The receive routine called by the interrupt handler
//
void INT0Serial::recv() {
    uint8_t d = 0;

    // If RX line is high, then we don't see any start bit
    // so interrupt is probably not for us
    if (!rx_pin_read()) {
        // Disable further interrupts during reception, this prevents
        // triggering another interrupt directly after we return, which can
        // cause problems at higher baudrates.
        setRxIntMsk(false);

        // Wait approximately 1/2 of a bit width to "center" the sample
        _delay_loop_2(_rx_delay_centering);

        // Read each of the 8 bits
        for (uint8_t i = 8; i > 0; --i) {
            _delay_loop_2(_rx_delay_intrabit);
            d >>= 1;
            if (rx_pin_read())
                d |= 0x80;
        }

        // if buffer full, set the overflow flag and return
        uint8_t next = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
        if (next != _receive_buffer_head) {
            // save new data in buffer: tail points to where byte goes
            _receive_buffer[_receive_buffer_tail] = (char)d;  // save new byte
            _receive_buffer_tail = next;
        } else {
            _buffer_overflow = true;
        }

        // skip the stop bit
        _delay_loop_2(_rx_delay_stopbit);

        // Re-enable interrupts when we're sure to be inside the stop bit
        setRxIntMsk(true);
    }
}

uint8_t INT0Serial::rx_pin_read() {
    return GPIO_READ(INT0_PORT, INT0_PIN);
}

//
// Interrupt handling
//

/* static */
inline void INT0Serial::handle_interrupt() {
    active_object->recv();
}

#if defined(INT0_vect)
ISR(INT0_vect) {
    INT0Serial::handle_interrupt();
}
#endif

//
// Constructor
//
INT0Serial::INT0Serial(uint8_t transmitPin) : _transmitPin(transmitPin), _rx_delay_centering(0), _rx_delay_intrabit(0), _rx_delay_stopbit(0), _tx_delay(0) {
    // setTX(transmitPin);
    //  First write, then set output. If we do this the other way around,
    //  the pin would be output low for a short while before switching to
    //  output high. Now, it is input with pullup for a short while, which
    //  is fine. With inverse logic, either order is fine.
    GPIO_SET_HIGH(INT0_PORT, _transmitPin);
    GPIO_OUTPUT(INT0_PORT, _transmitPin);

    // setRX(receivePin);
    GPIO_INPUT_PULLUP(INT0_PORT, INT0_PIN);

    active_object = this;
}

//
// Destructor
//
INT0Serial::~INT0Serial() {
    end();
}

uint16_t INT0Serial::subtract_cap(uint16_t num, uint16_t sub) {
    if (num > sub)
        return num - sub;
    else
        return 1;
}

//
// Public methods
//

void INT0Serial::begin(long speed) {
    _rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;

    // Precalculate the various delays, in number of 4-cycle delays
    uint16_t bit_delay = (F_CPU / speed) / 4;

    // 12 (gcc 4.8.2) or 13 (gcc 4.3.2) cycles from start bit to first bit,
    // 15 (gcc 4.8.2) or 16 (gcc 4.3.2) cycles between bits,
    // 12 (gcc 4.8.2) or 14 (gcc 4.3.2) cycles from last bit to stop bit
    // These are all close enough to just use 15 cycles, since the inter-bit
    // timings are the most critical (deviations stack 8 times)
    _tx_delay = subtract_cap(bit_delay, 15 / 4);

    // Timings counted from gcc 4.8.2 output. This works up to 115200 on
    // 16Mhz and 57600 on 8Mhz.
    //
    // When the start bit occurs, there are 3 or 4 cycles before the
    // interrupt flag is set, 4 cycles before the PC is set to the right
    // interrupt vector address and the old PC is pushed on the stack,
    // and then 75 cycles of instructions (including the RJMP in the
    // ISR vector table) until the first delay. After the delay, there
    // are 17 more cycles until the pin value is read (excluding the
    // delay in the loop).
    // We want to have a total delay of 1.5 bit time. Inside the loop,
    // we already wait for 1 bit time - 23 cycles, so here we wait for
    // 0.5 bit time - (71 + 18 - 22) cycles.
    _rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 75 + 17 - 23) / 4);

    // There are 23 cycles in each loop iteration (excluding the delay)
    _rx_delay_intrabit = subtract_cap(bit_delay, 23 / 4);

    // There are 37 cycles from the last bit read to the start of
    // stopbit delay and 11 cycles from the delay until the interrupt
    // mask is enabled again (which _must_ happen during the stopbit).
    // This delay aims at 3/4 of a bit time, meaning the end of the
    // delay will be at 1/4th of the stopbit. This allows some extra
    // time for ISR cleanup, which makes 115200 baud at 16Mhz work more
    // reliably
    _rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (37 + 11) / 4);

    // Enable the PCINT for the entire port here, but never disable it
    // (others might also need it, so we disable the interrupt by using
    // the per-pin PCMSK register).

    // Global Enable INT0 interrupt
    GICR |= (1 << INT0);
    // interrupt on failing edge
    MCUCR = (1 << ISC01);

    _delay_loop_2(_tx_delay);  // if we were low this establishes the end

    listen();
}

void INT0Serial::setRxIntMsk(bool enable) {
    if (enable)
        GICR |= (1 << INT0);
    else
        GICR &= ~(1 << INT0);
}

void INT0Serial::end() {
    stopListening();
}

// Read data from buffer
int INT0Serial::read() {
    // Empty buffer?
    if (_receive_buffer_head == _receive_buffer_tail)
        return -1;

    // Read from "head"
    uint8_t d = _receive_buffer[_receive_buffer_head];  // grab next byte
    _receive_buffer_head = (_receive_buffer_head + 1) % _SS_MAX_RX_BUFF;
    return d;
}

int INT0Serial::available() {
    return ((unsigned int)(_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head)) % _SS_MAX_RX_BUFF;
}

const char* INT0Serial::command() {
    const char* ret = (available() == 1) ? _receive_buffer + _receive_buffer_head : NULL;
    _buffer_overflow = 0;
    _receive_buffer_head = _receive_buffer_tail = 0;
    
    return ret;
  }

uint8_t INT0Serial::write(uint8_t b) {
    if (_tx_delay == 0) {
        return 0;
    }

    // By declaring these as local variables, the compiler will put them
    // in registers _before_ disabling interrupts and entering the
    // critical timing sections below, which makes it a lot easier to
    // verify the cycle timings
    uint8_t oldSREG = SREG;

    uint16_t delay = _tx_delay;

    cli();  // turn off interrupts for a clean txmit

    // Write the start bit
    //*reg &= inv_mask;
    GPIO_SET_LOW(INT0_PORT, _transmitPin);

    _delay_loop_2(delay);

    // Write each of the 8 bits
    for (uint8_t i = 8; i > 0; --i) {
        if (b & 1)  // choose bit
            //*reg |= reg_mask;  // send 1
            GPIO_SET_HIGH(INT0_PORT, _transmitPin);
        else
            //*reg &= inv_mask;  // send 0
            GPIO_SET_LOW(INT0_PORT, _transmitPin);

        _delay_loop_2(delay);
        b >>= 1;
    }

    // restore pin to natural state
    // *reg |= reg_mask;
    GPIO_SET_HIGH(INT0_PORT, _transmitPin);

    SREG = oldSREG;  // turn interrupts back on
    _delay_loop_2(_tx_delay);

    return 1;
}

int INT0Serial::peek() {
    // Empty buffer?
    if (_receive_buffer_head == _receive_buffer_tail)
        return -1;

    // Read from "head"
    return _receive_buffer[_receive_buffer_head];
}
