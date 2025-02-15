#ifndef INT0Serial_h
#define INT0Serial_h

#include <inttypes.h>
#include <util/delay_basic.h>


/******************************************************************************
 * Definitions
 ******************************************************************************/

#ifndef _SS_MAX_RX_BUFF
#define _SS_MAX_RX_BUFF 64  // RX buffer size
#endif

#define BAUD_RATE_9600 1
#define BAUD_RATE_57600 2
#define BAUD_RATE_115200 3

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class INT0Serial {
   private:
    // per object data
    uint8_t _receivePin;
    uint8_t _transmitPin;

    // Expressed as 4-cycle delays (must never be 0!)
    uint16_t _rx_delay_centering;
    uint16_t _rx_delay_intrabit;
    uint16_t _rx_delay_stopbit;
    uint16_t _tx_delay;

    static bool _buffer_overflow;

    // static data
    static char _receive_buffer[_SS_MAX_RX_BUFF];
    static volatile uint8_t _receive_buffer_tail;
    static volatile uint8_t _receive_buffer_head;

    // private methods
    inline void recv() __attribute__((__always_inline__));
    uint8_t rx_pin_read();

    inline void setRxIntMsk(bool enable) __attribute__((__always_inline__));

    // Return num - sub, or 1 if the result would be < 1
    static uint16_t subtract_cap(uint16_t num, uint16_t sub);

   public:
    // public methods

    // receive PIN is pin of INT0 interrupt, transmitPin shall be on the PORT close to INT0
    INT0Serial(uint8_t transmitPin);
    ~INT0Serial();
    void begin(long speed);
    bool listen();
    void end();
    bool stopListening();
    bool overflow() {
        bool ret = _buffer_overflow;
        if (ret) _buffer_overflow = false;
        return ret;
    }
    int peek();

    uint8_t write(uint8_t byte);
    int read();
    static int available();

    static const char *command();

    // public only for easy access by interrupt handlers
    static inline void handle_interrupt() __attribute__((__always_inline__));
};

#endif
