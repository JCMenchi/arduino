#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <millisec.h>

void init() {
  init_timer();
}

int main(void) {
  init();
  setup();

  for (;;) {
    loop();
  }

  return 0;
}