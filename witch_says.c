#include "utility.h"
#include "witch_says.h"
#include <kilombo.h>
#include <stdint.h>
#include <stdio.h>

REGISTER_USERDATA(USERDATA)
const uint32_t round_interval = 32 * 120;
const uint8_t num_bots = 5;
const uint8_t range_capture = 55;         // millimeters
const uint8_t avoid_collision_range = 45; // millimeters
const uint8_t witch_uid = 0;
const uint8_t delay_forwarding = 32;
const uint8_t max_delay_random = 255;
const uint8_t delay_last_adjustment = 32 * 3;

void setup_message(uint8_t msg_type, uint8_t color);
void message_rx(message_t *m, distance_measurement_t *d);
message_t *message_tx();
void restore_color();
void change_transmission(message_t m);
void blink(uint8_t delay);
void change_direction(uint8_t force_change);
void setup_message(uint8_t msg_type, uint8_t color);
void setup_init_flags();
void notify_capture(message_t *m);
int check_capture();
void avoid_collision(distance_measurement_t *d);
void game_over();

/*
Set the color of the kilobot according to its UID:
0:=RED - 1:=GREEN - 2:=BLUE - 3:=YELLOW - 4:=PURPLE
*/
void restore_color() {
  switch (kilo_uid) {
  case 0:
    set_color(RED);
    break;
  case 1:
    set_color(GREEN);
    break;
  case 2:
    set_color(BLUE);
    break;
  case 3:
    set_color(YELLOW);
    break;
  case 4:
    set_color(PURPLE);
    break;
  }
}

/*
Set the new transmission of the kilobot
*/
void change_transmission(message_t m) {
  if (m.data[0] == CHOSEN_COLOR)
    setup_message(m.data[0], m.data[1]);
  else
    setup_message(m.data[0], -1);
  kilo_message_tx = message_tx;
}

/*
Function handler for the event of receipt of a message.
There are 4 kind of message that can be received:
- CHOSEN_COLOR: the witcher sends the UID of the chosen kilobot
- IM_HERE: the runner sends periodically the signalling message
- GOTCHA: someone has caught the runner
- AVOID: every catcher must singnal its position in order to avoid collision
*/
void message_rx(message_t *m, distance_measurement_t *d) {
  switch (m->data[0]) {
  /******************CHOSEN_COLOR***********************/
  case CHOSEN_COLOR:
    if (mydata->time_over == 0)
      return;
    if (!mydata->CHOSEN_COLOR_received) {
      // event associated to CHOSEN_COLOR reception must be triggered only once
      // in a round
      printf("UID %d received CHOSEN_COLOR\n", kilo_uid);
      fflush(stdout);
      mydata->CHOSEN_COLOR_received = 1;
      if (kilo_uid == m->data[1]) {
        // the color of a kilobot is associated to its own UID
        // this is the runner, start sending IM_HERE
        m->data[0] = IM_HERE;
        m->data[1] = -1;
        mydata->im_runner = 1;
        mydata->start_timeout = kilo_ticks;
        mydata->last_change_direction = kilo_ticks;
        mydata->time_over = 1;
        set_motion(FORWARD);
        printf("UID %d, I'm the runner and I start to send IM_HERE\n", kilo_uid);
        fflush(stdout);
      }
      // every bot must forward the CHOSEN_COLOR message, except for
      // the runner which starts sending IM_HERE
      change_transmission(*m);
    }
    break;
  /***************************IM_HERE******************************/
  case IM_HERE:
    // received or not, CHOSEN_COLOR is considered received
    mydata->CHOSEN_COLOR_received = 1;
    if (!mydata->im_runner) {
      // runner should obviously ignore this message because it's the only
      // one that should send IM_HERE message
      if (mydata->is_first_IM_HERE) {
        printf("UID %d, start to forward IM_HERE\n", kilo_uid);
        fflush(stdout);
        mydata->start_timeout = kilo_ticks;
        mydata->time_over = 1;
        // bot must notify the others that the catch is started
        m->data[0] = IM_HERE;
        m->data[1] = -1;
        change_transmission(*m);
        mydata->is_first_IM_HERE = 0;
        mydata->forward_start_time = kilo_ticks;
      } else if (mydata->is_IM_HERE_sent) {
        // detect distance from the runner
        mydata->dist_old = mydata->dist_new;
        mydata->dist_new = *d;
        if (check_capture())
          notify_capture(m);
      }
    }
    break;
  /******************************GOTCHA***************************/
  case GOTCHA:
    // someone has got closer than 50mm and has caught the runner,
    // every bot must forward this message
    notify_capture(m);
    break;
  /******************************AVOID***************************/
  case AVOID:
    if (!mydata->im_runner && mydata->time_over != 0)
      avoid_collision(d);
    break;
  }
}

message_t *message_tx() { return &mydata->msg; }

/*
Routine that sets the appropriate color to simulate blinking,
according to the delay passed
*/
void blink(uint8_t delay) {
  if ((mydata->blink_last_time + delay) > kilo_ticks) {
    return;
  } else {
    // change the color and delay
    mydata->blink_flag = !mydata->blink_flag;
    mydata->blink_flag ? set_color(RGB(3, 3, 3)) : restore_color();
    // update the current time
    mydata->blink_last_time = kilo_ticks;
  }
}

/*
Routine periodically called to perform the following activities:
1) Check if the timeout of the current round is over
2) At the right moment, forward the message IM_HERE for a brief interval of time
3) Periodically calls the function blink()
4) Periodically calls the function change_direction()
*/
void loop() {
  // check round timeout
  if ((mydata->start_timeout + round_interval) < kilo_ticks &&
      mydata->time_over) {
    // time_over prevent to re-enter this statement again
    kilo_message_tx = empty_function_tx;
    kilo_message_rx = empty_function_rx;
    game_over();
  }

  // temporarily forwarding IM_HERE messages by catchers
  if (mydata->CHOSEN_COLOR_received && !mydata->im_runner &&
      !mydata->is_IM_HERE_sent &&
      (mydata->forward_start_time + delay_forwarding) < kilo_ticks) {
    // start the catch
    mydata->msg.data[0] = AVOID;
    change_transmission(mydata->msg);
    mydata->is_IM_HERE_sent = 1;
    mydata->last_change_direction = kilo_ticks;
    set_motion(FORWARD);
  }

  // blink section
  if (mydata->time_over == 0) {
    // round complete (either for catch or timeout), blinks to notify
    mydata->blink_flag ? blink(8) : blink(8);
  } else {
    if (mydata->im_runner)
      mydata->blink_flag ? blink(16) : blink(16);
    else if (kilo_uid == witch_uid)
      mydata->blink_flag ? blink(32) : blink(64);
  }

  if (mydata->time_over)
    change_direction(0);
}

/*
Allows a kilobot to randomly change the direction during the game
*/
void change_direction(uint8_t force_change) {
  if (force_change ||
      (mydata->delay_movement + mydata->last_change_direction) < kilo_ticks) {
    mydata->delay_movement = rand_hard() % max_delay_random;
    mydata->last_change_direction = kilo_ticks;
    if (mydata->movement) {
      // rotation, the direction is dependent on the value of the delay
      (mydata->delay_movement % 2) == 0 ? set_motion(LEFT) : set_motion(RIGHT);
    } else {
      // go straight forward
      set_motion(FORWARD);
    }
    mydata->movement = !mydata->movement;
  }
}

/*
If the distance detected is less than a specified
threshold, then the kilobot stops and terminate the catching.
*/
void avoid_collision(distance_measurement_t *d) {
  if (estimate_distance(d) < avoid_collision_range) {
    // it's to dangerous to move due to a possible collision, stop and continue
    // to forward AVOID to signal the position
    mydata->msg.data[0] = AVOID;
    change_transmission(mydata->msg);
    game_over();
    printf("UID: %d, stop to avoid a collision.\n", kilo_uid);
    fflush(stdout);
  } else {
    change_direction(1);
  }
}

/*
Whenever a kilobot capture the witcher, it starts to sends a notification
message GOTCHA and stops moving
*/
void notify_capture(message_t *m) {
  if (!mydata->im_runner)
    printf("UID: %d, the runner has been caught.\n", kilo_uid);
  else
    printf("UID: %d, I've been caught.\n", kilo_uid);
  fflush(stdout);
  kilo_message_rx = empty_function_rx;
  m->data[0] = GOTCHA;
  m->data[1] = -1;
  change_transmission(*m);
  game_over();
}

/*
Estimate the two final distances detected from the runner: if both of them
are less than a specified threshold, then the runner is considered captured.
Otherwise it controls if the kilobot is moving away from the target, in that
case it forces a change of direction
*/
int check_capture() {
  uint32_t distance_new = estimate_distance(&mydata->dist_new);
  uint32_t distance_old = estimate_distance(&mydata->dist_old);
  if (distance_new <= range_capture && distance_old <= range_capture) {
    printf("UID: %d, I capture the runner.\n", kilo_uid);
    fflush(stdout);
    return 1;
  } else {
    // if a force change direction has occured, wait at least some seconds
    // before attempting again an adjustment
    if (mydata->time_over && distance_new < distance_old &&
        (mydata->last_change_direction + delay_last_adjustment) < kilo_ticks) {
      // the bot is walking away from runner, force a change of direction
      change_direction(1);
    }
    return 0;
  }
}

/*
Set the new message to transmit
*/
void setup_message(uint8_t msg_type, uint8_t color) {
  mydata->msg.type = NORMAL;
  mydata->msg.data[0] = msg_type & 0xff;
  if (msg_type == CHOSEN_COLOR)
    mydata->msg.data[1] = color & 0xff;
  else
    mydata->msg.data[1] = -1;
  mydata->msg.crc = message_crc(&mydata->msg);
}

/*
Restore the initial configuration of the system
*/
void setup_init_flags() {
  mydata->delay_movement = max_delay_random;
  mydata->blink_last_time = kilo_ticks;
  mydata->blink_flag = 0;
  mydata->CHOSEN_COLOR_received = 0;
  mydata->im_runner = 0; // no one is the runner at the beginning of the round
  mydata->is_first_IM_HERE = 1;
  mydata->is_IM_HERE_sent = 0;
  mydata->time_over = 1;
}

/*
The witcher chooses randomly a UID that will be the color of the runner
and notifies all the adjacent kilobots
*/
void setup() {
  setup_init_flags();
  // the witch randomly chooses a color and starts the communication
  uint8_t color;
  if (kilo_uid == witch_uid) {
    do {
      color = rand_hard() % num_bots;
      printf("CHOSEN %d\n", color);
      fflush(stdout);
    } while (color == kilo_uid);
    setup_message(CHOSEN_COLOR, color);
    kilo_message_tx = message_tx;
  }
  restore_color();
}

/*
Stops the kilobot and terminate the round for that kilobot
*/
void game_over() {
  set_motion(STOP);
  mydata->time_over = 0;
}

int main() {
  kilo_init();
  kilo_message_rx = message_rx;
  kilo_start(setup, loop);
  return 0;
}
