#include <kilombo.h>

#define RED RGB(3,0,0)
#define GREEN RGB(0,3,0)
#define BLUE RGB(0,0,3)
#define PURPLE RGB(3,0,3)
#define YELLOW RGB(3,3,0)

typedef enum {
    CHOSEN_COLOR,
    IM_HERE,
    GOTCHA,
    AVOID
}message_game_t;

typedef struct
{
    message_t msg;
    distance_measurement_t dist_old; // second-last distance estimante for the runner
    distance_measurement_t dist_new;// last distance estimante for the runner
    uint8_t delay_movement; // represent the interval of time randomly selected to perform the movement
    uint32_t last_change_direction; // last timestamp in which occured a change of direction
    uint8_t movement; // flag: 0:=make a rotation, otherwise go straight forward
    uint32_t blink_last_time; // timestamp of the last change of the color
    uint8_t blink_flag; // alternate the colors
    uint8_t CHOSEN_COLOR_received; //flag for determine if CHOSEN_COLOR has been received in the current round
    uint8_t im_runner; //true if the bot is the runner
    uint32_t forward_start_time; //when a bot has to forward a message for a predefined interval, it sets the start time
    uint8_t is_first_IM_HERE; //true if the bot has not received a IM_HERE yet
    uint8_t is_IM_HERE_sent; // true if the bot has been forwarding IM_HERE for an interval of time
    uint32_t start_timeout; // timestamp of the start of the game for a single bot
    uint8_t time_over; // true if the round is terminated for this bot, either for catch or timeout
} USERDATA;
