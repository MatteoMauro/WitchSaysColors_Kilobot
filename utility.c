#include "utility.h"
#include <kilombo.h>
#include <stdint.h>

/* Perform a smoothly motor speed
 */
void smooth_set_motors(uint8_t ccw, uint8_t cw) {
//OCR2A = ccw;  OCR2B = cw;
#ifdef KILOBOT
  uint8_t l = 0, r = 0;
  if (ccw && !OCR2A) // we want left motor on, and it's off
    l = 0xff;
  if (cw && !OCR2B) // we want right motor on, and it's off
    r = 0xff;
  if (l || r) // at least one motor needs spin-up
  {
    set_motors(l, r);
    delay(15);
  }
#endif
  // spin-up is done, now we set the real value
  set_motors(ccw, cw);
}

/* Set a direction for the next movement
 */
void set_motion(motion_t new_motion) {
  switch (new_motion) {
  case STOP:
    smooth_set_motors(0, 0);
    break;
  case FORWARD:
    smooth_set_motors(kilo_turn_left, kilo_turn_right);
    break;
  case LEFT:
    smooth_set_motors(kilo_turn_left, 0);
    break;
  case RIGHT:
    smooth_set_motors(0, kilo_turn_right);
    break;
  }
}

message_t *empty_function_tx() { return NULL; }
void empty_function_tx_success() {}
void empty_function_rx(message_t *m, distance_measurement_t *d) {}
