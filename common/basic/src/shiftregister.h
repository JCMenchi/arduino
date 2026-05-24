/*
    shiftregister.h - Library for controlling a shift register.
    This library is designed to work with 74HC595 shift registers.
    It allows you to control up to 8 outputs using 3 pins from the microcontroller.

    The shift register is connected as follows:
                 +----------+
                 |°         |
  Output <-- QB--|1   S   16|--VCC
                 |    H     |
  Output <-- QC--|2   I   15|--QA    --> Output
                 |    F     |
  Output <-- QD--|3   T   14|--DS    <-- MCU
                 |          |
  Output <-- QE--|4   R   13|--OE    --> GND
                 |    E     |
  Output <-- QF--|5   G   12|--STCP  <-- MCU
                 |    I     |
  Output <-- QG--|6   S   11|--SHCP  <-- MCU
                 |    T     |
  Output <-- QH--|7   E   10|--Reset --> VCC
                 |    R     |
            GND--|8        9|--NotConnected
                 +----------+

Pinout:
    QA–QH: Parallel outputs
    DS: Serial Data Input
    SHCP: Shift Register Clock Input
    STCP: Storage Register Clock Input (Latch)
    OE: Output Enable (active low)
    MR: Master Reset (active low)
    VCC: Power
    GND: Ground
    NC: Not Connected

*/

#ifndef shiftregister_h
#define shiftregister_h

#include <gpio.h>
#include <string.h>

#ifndef SHREG_PORTID
#define SHREG_PORTID A
#endif

#define SR_Q0 0
#define SR_Q1 1
#define SR_Q2 2
#define SR_Q3 3
#define SR_Q4 4
#define SR_Q5 5
#define SR_Q6 6
#define SR_Q7 7

#define SR_Q0_MASK 0x01
#define SR_Q1_MASK 0x02
#define SR_Q2_MASK 0x04
#define SR_Q3_MASK 0x08
#define SR_Q4_MASK 0x10
#define SR_Q5_MASK 0x20
#define SR_Q6_MASK 0x40
#define SR_Q7_MASK 0x80

#define SR_QA 0
#define SR_QB 1
#define SR_QC 2
#define SR_QD 3
#define SR_QE 4
#define SR_QF 5
#define SR_QG 6
#define SR_QH 7

#define SR_QA_MASK 0x01
#define SR_QB_MASK 0x02
#define SR_QC_MASK 0x04
#define SR_QD_MASK 0x08
#define SR_QE_MASK 0x10
#define SR_QF_MASK 0x20
#define SR_QG_MASK 0x40
#define SR_QH_MASK 0x80

/**
 * @brief A class to control an 8-bit shift register (e.g., 74HC595)
 *
 * This class provides convenient methods to control an 8-bit shift register using three
 * microcontroller pins: data (serial input), latch (storage register clock), and clock
 * (shift register clock). It maintains an internal state array representing the current
 * state of all 8 output bits and automatically synchronizes them with the shift register.
 *
 * The class handles the timing and signaling protocol required by the 74HC595 shift
 * register, allowing you to manipulate individual output bits or set all 8 bits at once.
 * All control pins must reside on the same GPIO port as defined by SHREG_PORTID macro
 * (defaults to PORTA).
 *
 * @note Before using any other methods, call setup() to initialize the control pins as
 *       outputs and set appropriate initial states.
 * @note The class does not support multiple shift registers chained in series.
 * @note Output changes only take effect when sendData() is called.
 *
 * Example usage:
 * @code
 * // Create instance with pins on PORTA: data=PA0, latch=PA1, clock=PA2
 * ShiftRegisterPort sr(0, 1, 2);
 * sr.setup();                     // Initialize pins
 * sr.setState(0, 1);              // Set output Q0 high
 * sr.setState(1, 1);              // Set output Q1 high
 * sr.sendData();                  // Update shift register with new state
 * sr.allHigh();                   // Set all 8 outputs high
 * sr.allLow();                    // Set all 8 outputs low
 * uint8_t mask = sr.getMask();    // Get current state as bitmask
 * sr.setMask(0xFF);               // Set all 8 bits from a bitmask
 * @endcode
 *
 * @see https://www.ti.com/lit/ds/symlink/sn74hc595.pdf for 74HC595 datasheet
 */
class ShiftRegisterPort {
   public:
    /**
     * @brief Constructs a ShiftRegisterPort instance with specified control pins.
     *
     * All pins must be on the same GPIO port as defined by SHREG_PORTID macro.
     * The internal state is initialized to all zeros (all outputs low).
     *
     * @param[in] data  Pin number for serial data input (DS)
     * @param[in] latch Pin number for storage register clock (STCP/latch)
     * @param[in] clock Pin number for shift register clock (SHCP)
     *
     * @note Call setup() after construction to initialize the pins for output.
     */
    ShiftRegisterPort(uint8_t data, uint8_t latch, uint8_t clock)
        : dataPin(data), latchPin(latch), clockPin(clock) {
        memset(state, 0, 8);
    }

    /**
     * @brief Initializes the control pins for output and sets appropriate initial states.
     *
     * This method must be called once after construction and before using any other
     * methods. It configures the data, latch, and clock pins as GPIO outputs and sets
     * them to their inactive states:
     * - latch pin: HIGH (inactive/holding state)
     * - data pin: LOW (default)
     * - clock pin: HIGH (inactive)
     */
    void setup() {
        GPIO_SET_HIGH(SHREG_PORTID, this->latchPin);
        GPIO_SET_LOW(SHREG_PORTID, this->dataPin);
        GPIO_SET_HIGH(SHREG_PORTID, this->clockPin);

        GPIO_OUTPUT(SHREG_PORTID, this->dataPin);
        GPIO_OUTPUT(SHREG_PORTID, this->latchPin);
        GPIO_OUTPUT(SHREG_PORTID, this->clockPin);
    }

    /**
     * @brief Toggles the state of a single output bit.
     *
     * Flips the specified bit between HIGH (1) and LOW (0). The change takes effect
     * only when sendData() is called.
     *
     * @param[in] pos Bit position to toggle (0-7, where 0 = QA, 7 = QH)
     * @return void
     *
     * @note Invalid positions (>= 8) are silently ignored.
     */
    void toggleBit(uint8_t pos) {
        if (pos >= 8) {
            return;
        }

        if (state[pos] == 0) {
            state[pos] = 1;
        } else {
            state[pos] = 0;
        }
    }

    /**
     * @brief Internal helper method that shifts the current state into the shift register.
     *
     * This method serially clocks out the 8 bits from the internal state array into
     * the shift register's serial input, from MSB (bit 7) to LSB (bit 0). It does not
     * latch the data or update the outputs; call sendData() for complete updates.
     *
     * @return void
     *
     * @note This is a private implementation detail called by sendData().
     * @note Generates 8 clock pulses on the clock pin.
     */
    void writeRegister(uint8_t val) {
        int8_t i;

        for (i = 7; i >= 0; i--) {
            if (this->state[i] != 0) {
                GPIO_SET_HIGH(SHREG_PORTID, this->dataPin);
            } else {
                GPIO_SET_LOW(SHREG_PORTID, this->dataPin);
            }

            GPIO_SET_HIGH(SHREG_PORTID, this->clockPin);
            GPIO_SET_LOW(SHREG_PORTID, this->clockPin);
        }
    }

    /**
     * @brief Sends the current internal state to the shift register outputs.
     *
     * This method transfers the 8-bit internal state to the shift register and
     * latches the data to update all outputs simultaneously. It performs the following:
     * 1. Converts the internal state array to an 8-bit mask
     * 2. Pulls latch LOW to protect outputs during transfer
     * 3. Serially clocks all 8 bits into the shift register
     * 4. Pulls latch HIGH to transfer the data to outputs
     * 5. Clears the data line
     *
     * Call this method after modifying the internal state (via setState, toggleBit,
     * or setMask) to update the shift register outputs.
     *
     * @return void
     *
     * @note Without calling this method, changes to the internal state will not
     *       affect the physical shift register outputs.
     */
    void sendData() {
        uint8_t v = this->getMask();

        // take the latchPin low so
        // the LEDs don't change while we are sending in bits:
        GPIO_SET_LOW(SHREG_PORTID, this->latchPin);
        // shift out the bits:
        writeRegister(v);
        // take the latch pin high so the LEDs will light up:
        GPIO_SET_HIGH(SHREG_PORTID, this->latchPin);

        GPIO_SET_LOW(SHREG_PORTID, this->dataPin);
    }

    /**
     * @brief Sets all 8 output bits from a single byte mask and updates the shift register.
     *
     * Interprets the provided byte as a bitmask where bit 0 corresponds to QA output,
     * bit 1 to QB, and so on (bit 7 to QH). Automatically calls sendData() to update
     * the shift register outputs.
     *
     * @param[in] v Bitmask to set (bit 0 = QA, bit 1 = QB, ..., bit 7 = QH)
     * @return void
     *
     * @note This is a convenience method equivalent to calling setState() 8 times
     *       followed by sendData().
     *
     * Example:
     * @code
     * sr.setMask(0b10101010);  // Alternating pattern on QA-QH
     * sr.setMask(0xFF);         // All outputs HIGH
     * sr.setMask(0x00);         // All outputs LOW
     * @endcode
     */
    void setMask(uint8_t v) {
        for (int i = 0; i < 8; i++) {
            this->state[i] = v & (1 << i);
        }

        this->sendData();
    }

    /**
     * @brief Returns the current internal state as an 8-bit bitmask.
     *
     * Converts the internal state array into a single byte where each bit represents
     * the state of the corresponding output: bit 0 = QA, bit 1 = QB, ..., bit 7 = QH.
     * Note that this returns the internal state, not the actual shift register outputs
     * (though they should match after sendData() is called).
     *
     * @return uint8_t Bitmask representing current state (bit i set means output Qi is HIGH)
     */
    uint8_t getMask() const {
        uint8_t v = 0;

        for (int i = 0; i < 8; i++) {
            if (this->state[i] == 1) {
                v |= (1 << i);
            }
        }

        return v;
    }

    /**
     * @brief Sets the state of a single output bit without updating the shift register.
     *
     * Modifies the internal state of the specified bit. To make this change visible
     * on the shift register outputs, call sendData() after setting state(s).
     *
     * @param[in] pos   Bit position to modify (0-7, where 0 = QA, 7 = QH)
     * @param[in] value New state for the bit (0 = LOW, non-zero = HIGH)
     * @return void
     *
     * @note Invalid positions (>= 8) are silently ignored.
     * @note The value parameter is treated as a boolean: 0 = LOW, non-zero = HIGH.
     * @note Changes only take effect on shift register after calling sendData().
     *
     * Example:
     * @code
     * sr.setState(0, 1);    // Set QA HIGH
     * sr.setState(1, 0);    // Set QB LOW
     * sr.setState(7, 1);    // Set QH HIGH
     * sr.sendData();        // Update shift register outputs
     * @endcode
     */
    void setState(uint8_t pos, uint8_t value) {
        if (pos >= 8) {
            return;
        }

        this->state[pos] = value;
    }

    /**
     * @brief Returns the internal state of a single output bit.
     *
     * Retrieves the current internal state of the specified bit without modifying
     * the shift register or any other state. Returns the internal state value,
     * not necessarily the actual shift register output (though they should match
     * after sendData() is called).
     *
     * @param[in] pos Bit position to query (0-7, where 0 = QA, 7 = QH)
     * @return uint8_t State of the bit (0 = LOW, non-zero = HIGH)
     *
     * @note Invalid positions (>= 8) return 0.
     *
     * Example:
     * @code
     * uint8_t state = sr.getState(0);  // Get state of QA
     * if (sr.getState(3) == 0) {
     *     // QD is currently LOW
     * }
     * @endcode
     */
    uint8_t getState(uint8_t pos) const {
        if (pos >= 8) {
            return 0;
        }

        return this->state[pos];
    }

    /**
     * @brief Sets all 8 output bits LOW and updates the shift register.
     *
     * Convenience method to quickly set all outputs to LOW (0). Equivalent to calling
     * setState(i, 0) for all 8 bits and then calling sendData().
     *
     * @return void
     *
     * Example:
     * @code
     * sr.allLow();  // Turn off all 8 outputs immediately
     * @endcode
     */
    void allLow() {
        for (int i = 0; i < 8; i++) {
            setState(i, 0);
        }
        sendData();
    }

    /**
     * @brief Sets all 8 output bits HIGH and updates the shift register.
     *
     * Convenience method to quickly set all outputs to HIGH (1). Equivalent to calling
     * setState(i, 1) for all 8 bits and then calling sendData().
     *
     * @return void
     *
     * Example:
     * @code
     * sr.allHigh();  // Turn on all 8 outputs immediately
     * @endcode
     */
    void allHigh() {
        for (int i = 0; i < 8; i++) {
            setState(i, 1);
        }
        sendData();
    }

   private:
    /** @brief GPIO pin number for serial data input (DS) */
    uint8_t dataPin;

    /** @brief GPIO pin number for storage register clock/latch (STCP) */
    uint8_t latchPin;

    /** @brief GPIO pin number for shift register clock (SHCP) */
    uint8_t clockPin;

    /**
     * @brief Internal state array representing the 8 output bits.
     *
     * Each element is either 0 (LOW) or non-zero (HIGH), corresponding to outputs
     * QA through QH (indices 0-7). The actual shift register outputs are only
     * updated when sendData() is called.
     */
    uint8_t state[8];
};

#endif  // shiftregister_h
