#include "gpio.h"
#include "timer.h"

static const unsigned int SEGMENT_PIN_FIRST = GPIO_PIN20;
static const unsigned int N_SEGMENT_PINS = 8;

static const unsigned int POSITION_PIN_FIRST = GPIO_PIN10;
static const unsigned int N_POSITION_PINS = 4;

static const unsigned int SWITCH_BUTTON_PIN = GPIO_PIN2;
static const unsigned int SET_BUTTON_PIN = GPIO_PIN3;

/*
 * Controls which decimal value (0 - 9) will be displayed in each
 * clock position, where `display[0]` is the leftmost physical position
 */
static char display[4];

// Clock modes
enum {
    MODE_DEFAULT = 0,
    MODE_COUNTUP = 1,
    MODE_SET = 2
};

// Set minutes vs. set seconds in select mode
enum {
    MIN_SELECT = 0,
    SEC_SELECT = 1
};

/*
 * Encodes segment patterns to display digits and letters
 * in an order such that `alphanum_patterns[n]` corresponds
 * to the hex value of `n` (e.g. `alphanum_patterns[0xA]` --> A)
 */
static const char alphanum_patterns[] = {
    0b00111111, 0b00000110, 0b01011011, 0b01001111,
    0b01100110, 0b01101101, 0b01111101, 0b00000111,
    0b01111111, 0b01100111, 0b01110111, 0b01111100,
    0b00111001, 0b01011110, 0b01111001, 0b01110001
};

/*
 * Sets segment GPIO pins to display the pattern encoded for
 * by `pattern`.
 *
 * @param pattern   the pattern to display
 */
static void set_pattern(char pattern) {
    for (int i = 0; i < N_SEGMENT_PINS; i++) {
        gpio_write(SEGMENT_PIN_FIRST + i, (pattern >> i) & 1);
    }
}

static const unsigned int MICROS_PER_MILLI = 1000;
static const unsigned int MICROS_PER_SEC = 1000000;

static const unsigned int SEC_MAX = 60;
static const unsigned int MIN_MAX = 100;

/*
 * Returns as unsigned int the number of minutes to display
 * on the clock (at time of function call) given a start time in microseconds.
 *
 * @param start_time_us     the time when the clock initialized in microseconds since Pi startup
 * @return                  minutes to display on clock in decimal
 */
static unsigned int clock_get_mins(unsigned int start_time_us) {
    unsigned int elapsed_time_us = timer_get_ticks() - start_time_us;
    unsigned int secs = elapsed_time_us / MICROS_PER_SEC;
    unsigned int mins = secs / 60;
    mins %= MIN_MAX; // Maximum # mins checked by spec; modding prevents overflow
    return mins;
}

/*
 * Returns as unsigned int the number of seconds to display
 * on the clock (at time of function call) given a start time in microseconds.
 *
 * @param start_time_us     the time when the clock initialized in microseconds since Pi startup
 * @return                  seconds to display on clock in decimal
 */
static unsigned int clock_get_secs(unsigned int start_time_us) {
    unsigned int elapsed_time_us = timer_get_ticks() - start_time_us;
    unsigned int secs = elapsed_time_us / MICROS_PER_SEC;
    secs %= SEC_MAX;
    return secs;
}

/*
 * Sets the display value for each digit in the clock.
 * 
 * @param mins      current time to set clock to, in minutes
 * @param secs      current time to set clock to, in seconds
 */
static void clock_set_display(unsigned int mins, unsigned int secs) {
    display[0] = (mins / 10) % 10;
    display[1] = mins % 10;
    display[N_POSITION_PINS - 2] = (secs / 10) % 10;
    display[N_POSITION_PINS - 1] = secs % 10;
}

struct Button {
    int was_pressed;
    int is_pressed;
    // Bool; nonzero if respective a press has been registered (debounce-corrected) and needs processing
    int needs_processing;
};

static const char WAIT_PATTERN = 0b01000000;
static const unsigned int MULTIPLEX_DELAY_US = 2500;
static const unsigned int DEBOUNCE_TIME_MS = 70;
static const unsigned int BLINK_FREQ_MS = 500;
static const unsigned int BLINK_OFF_PERCENT = 35; // Out of 100 max

void main(void)
{
    // Initialize button GPIO pins as input
    gpio_set_input(SWITCH_BUTTON_PIN);
    gpio_set_input(SET_BUTTON_PIN);

    // Initialize clock GPIO pins as output
    for (int i = 0; i < N_SEGMENT_PINS; i++) {
        gpio_set_output(SEGMENT_PIN_FIRST + i);
    }
    for (int i = 0; i < N_POSITION_PINS; i++) {
        gpio_set_output(POSITION_PIN_FIRST + i);
        gpio_write(POSITION_PIN_FIRST + i, 0);
        display[i] = 0;
    }

    int clock_mode = MODE_DEFAULT;
    unsigned int start_time_us;
    // State variables for setting clock
    unsigned int set_mins, set_secs;
    unsigned int set_select = SEC_SELECT;

    struct Button b_switch, b_set;
    b_switch.was_pressed = b_switch.is_pressed = b_switch.needs_processing = 0;
    b_set.was_pressed = b_set.is_pressed = b_set.needs_processing = 0;
    unsigned int last_button_check_us = timer_get_ticks(); // For debounce
    unsigned int set_blink_offset; // Blink mins or secs, whichever is currently being set
    unsigned int blink_off = 0; // When in set mode, determines whether to turn off mins/seconds for "blinking" effect

    // Main loop
    while (1) {

        // Read button presses and update button states
        unsigned int elapsed_since_check = timer_get_ticks() - last_button_check_us;
        if (elapsed_since_check > DEBOUNCE_TIME_MS * MICROS_PER_MILLI) {
            b_switch.is_pressed = gpio_read(SWITCH_BUTTON_PIN) == 0;
            b_set.is_pressed = gpio_read(SET_BUTTON_PIN) == 0;

            // Switch button
            if (b_switch.needs_processing == 0) {
                b_switch.needs_processing = !b_switch.was_pressed && b_switch.is_pressed;
            }
            b_switch.was_pressed = b_switch.is_pressed;

            // Set button
            if (b_set.needs_processing == 0) {
                b_set.needs_processing = !b_set.was_pressed && b_set.is_pressed;
            }
            b_set.was_pressed = b_set.is_pressed;

            last_button_check_us = timer_get_ticks();
        }


        // Update clock mode and state (factoring in button presses that need to be processed)
        if (clock_mode == MODE_DEFAULT) {

            if (b_switch.needs_processing != 0) {
                // Initialize clock model
                start_time_us = timer_get_ticks();
                clock_mode = MODE_COUNTUP;

                // Ignore simultaneous 2 button press and clear queue so it isn't mysteriously handled later
                b_set.needs_processing = 0;
                // Mark as processed
                b_switch.needs_processing = 0;

            } else if (b_set.needs_processing != 0) {
                clock_mode = MODE_SET;

                set_mins = set_secs = 0;
                clock_set_display(set_mins, set_mins);
                set_select = SEC_SELECT;
                set_blink_offset = timer_get_ticks();

                // Ignore simultaneous 2 button press and clear queue so it isn't mysteriously handled later
                b_switch.needs_processing = 0;
                // Mark as processed
                b_set.needs_processing = 0;
            }

        } else if (clock_mode == MODE_COUNTUP) {

            // Update clock display
            unsigned int secs = clock_get_secs(start_time_us);
            unsigned int mins = clock_get_mins(start_time_us);
            clock_set_display(mins, secs);

            if (b_set.needs_processing != 0) {
                clock_mode = MODE_SET;
                set_mins = mins;
                set_secs = secs;
                set_blink_offset = timer_get_ticks();

                // Mark as processed
                b_set.needs_processing = 0;
            }

        } else { // clock_mode == MODE_SET

            if (b_switch.needs_processing != 0 && b_set.needs_processing != 0) {
                // Lock in new time by getting current time and back-calculating artificial "start time"
                clock_set_display(set_mins, set_secs);
                unsigned int elapsed_time_us = (set_mins * 60 + set_secs) * MICROS_PER_SEC;
                start_time_us = timer_get_ticks() - elapsed_time_us;
                clock_mode = MODE_COUNTUP;

                b_switch.needs_processing = b_set.needs_processing = 0;
            } else if (b_switch.needs_processing != 0) {
                set_select = (set_select == MIN_SELECT) ? SEC_SELECT : MIN_SELECT;

                // Mark as processed
                b_switch.needs_processing = 0;
            } else if (b_set.needs_processing != 0) {
                // Increment minutes / seconds
                if (set_select == SEC_SELECT) {
                    set_secs++;
                    set_secs %= SEC_MAX;
                } else {
                    set_mins++;
                    set_mins %= MIN_MAX;
                }
                clock_set_display(set_mins, set_secs);

                // Mark as processed
                b_set.needs_processing = 0;
            }

            // Blink current field being set (mins or secs)
            unsigned int set_elapsed_ms = (timer_get_ticks() - set_blink_offset) / MICROS_PER_MILLI;
            set_elapsed_ms %= BLINK_FREQ_MS;
            blink_off = (set_elapsed_ms <= ((BLINK_FREQ_MS * BLINK_OFF_PERCENT) / 100)); // Will be true `BLINK_OFF_PERCENT` percent of the time
        }


        // Multiplex (i.e. display what should currently be on the clock)
        for (int i = 0; i < N_POSITION_PINS; i++) {
            if (clock_mode == MODE_SET && blink_off) {
                // We should blink mins or seconds (i.e. sometimes don't turn on the corresponding pins)
                if ((set_select == MIN_SELECT && i < 2) ||
                    (set_select == SEC_SELECT && i >= 2))  {
                    timer_delay_us(MULTIPLEX_DELAY_US);
                    continue;
                }
            }

            char pattern = (clock_mode == MODE_DEFAULT) ?
                WAIT_PATTERN : alphanum_patterns[(unsigned int)display[i]];

            set_pattern(pattern);
            gpio_write(POSITION_PIN_FIRST + i, 1);      
            timer_delay_us(MULTIPLEX_DELAY_US);
            gpio_write(POSITION_PIN_FIRST + i, 0);
        }
    }
}
