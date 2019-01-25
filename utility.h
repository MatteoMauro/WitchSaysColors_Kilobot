#include <stdint.h>
#include <kilombo.h>

// declare motion variable type
typedef enum {
        STOP,
        FORWARD,
        LEFT,
        RIGHT
} motion_t;

/* Set a smoothly motor speed
   ccw:= counter-clockwise
   cw:= clockwise
 */
void smooth_set_motors(uint8_t ccw, uint8_t cw);

/* Set a direction for the next movement
 */
void set_motion(motion_t direction);

message_t *empty_function_tx();
void empty_function_tx_success();
void empty_function_rx(message_t *m, distance_measurement_t *d);
