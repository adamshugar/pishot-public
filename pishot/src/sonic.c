#include "countdown.h"
#include "gpio.h"
#include "gpioextra.h"
#include "malloc.h"
#include "sonic.h"
#include "sonic_rb.h"
#include "strings.h"
#include "timer.h"

/*
 * Written by Adam Shugar on March 11, 2020.
 */

// All times in state struct are in microseconds
struct sonic_config {
    sonic_sensor_t *sensors;                // Array of sensor metadata (maps GPIO pins to each sensor)
    int n_sensors;                          // Total number of sensors (size of `sensors` arr)
    sonic_rb_t *arr_readings;               // ASYNC ONLY: Cumulative collection of sensor array data for all past loops
    sonic_data_t *curr_data;                // ASYNC ONLY: Array of sensor data for current loop (maps to sensor arr)
    int curr_sensor;                        // ASYNC ONLY: Index of current sensor
    unsigned curr_trigger_timestamp;        // ASYNC ONLY: Timestamp of curr reading start to calculate dt
    bool awaiting_echo;                     // ASYNC ONLY: Whether we have sent a trigger and are awaiting response
    bool is_active;                         // ASYNC ONLY: Whether entire module is "on" or "off"
    bool handler_attached;                  // ASYNC ONLY: Flag turned on after interrupt handler attached to prevent duplicates
    unsigned cycle_delay;                   // Delay between end of 1st full-arr reading and start of 2nd
    unsigned unit_delay;                    // Delay between each sensor reading
    unsigned timeout;                       // Max time to wait before moving to next sensor reading
};
static struct sonic_config state;

static void start_timer(unsigned micros, handler_fn_t callback)
{
    // Assume that while this module is active the client is not
    // setting other interrupt-based timers or otherwise using the
    // ARM timer module.
    countdown_set_ticks(micros);
    countdown_set_handler(callback);
    countdown_enable();
}

// ---------------- BEGIN READ LOOP FUNCTIONS ----------------
// Forward references for callbacks
static bool finish_trigger(unsigned int);
static bool timeout(unsigned int);
static void next_sensor(void);
// Drive trigger pin on HC-SR04 high for >= 10 microsecs to send a pulse per datasheet
#define TRIGGER_DELAY 10
static bool start_trigger(unsigned int pc)
{
    // Break out of the interrupt-based loop at beginning of routine
    // for a single sensor if client turned module off
    if (!state.is_active) return true;
    gpio_write(state.sensors[state.curr_sensor].trigger, 1);
    start_timer(TRIGGER_DELAY, finish_trigger);
    return true;
}

static bool finish_trigger(unsigned int pc)
{ 
    if (!state.is_active) return true;

    gpio_write(state.sensors[state.curr_sensor].trigger, 0);
    state.curr_trigger_timestamp = timer_get_ticks();
    state.awaiting_echo = true;
    start_timer(state.timeout, timeout);
    return true;
}

static bool timeout(unsigned int pc)
{
    if (!state.awaiting_echo) return true;

    state.awaiting_echo = false;
    state.curr_data[state.curr_sensor].distance = SONIC_INVALID_READING;
    state.curr_data[state.curr_sensor].timestamp = state.curr_trigger_timestamp;
    next_sensor();
    return true;
}

static void next_sensor(void)
{
    if (!state.is_active) return;

    unsigned delay;
    if (state.curr_sensor == state.n_sensors - 1) {
        sonic_data_t *complete_cycle = malloc(sizeof(sonic_data_t) * state.n_sensors);
        memcpy(complete_cycle, state.curr_data, sizeof(sonic_data_t) * state.n_sensors);
        bool success = sonic_rb_enqueue(state.arr_readings, complete_cycle);
        if (!success) free(complete_cycle);
        state.curr_sensor = 0;
        // Delay for cycle time before starting new cycle
        delay = state.cycle_delay < SONIC_MIN_DELAY ? state.unit_delay : state.cycle_delay;
    } else {
        state.curr_sensor++;
        delay = state.unit_delay;
    }
    start_timer(delay, start_trigger);
}
// ---------------- END READ LOOP FUNCTIONS ----------------


#define SPEED_MM_MICRO .343 // Speed of sound in mm per microsecond
static bool process_echo(unsigned pc)
{
    // Only handle cases meant for this module
    if (!state.is_active) return false;

    // Noise on a pin; ignore event
    if (!state.awaiting_echo) {
        for (int i = 0; i < state.n_sensors; i++) {
            gpio_clear_event(state.sensors[i].echo);
        }
        return true;
    }

    unsigned echo_timestamp = timer_get_ticks();
    unsigned elapsed = echo_timestamp - state.curr_trigger_timestamp;
    // Pulse travelled there and back (hence div. 2)
    state.curr_data[state.curr_sensor].distance = (int) ((elapsed * SPEED_MM_MICRO) / 2);
    // Pulse hit object at halfway between start and end timestamps
    state.curr_data[state.curr_sensor].timestamp = state.curr_trigger_timestamp + (elapsed / 2);

    state.awaiting_echo = false;
    gpio_clear_event(state.sensors[state.curr_sensor].echo);
    next_sensor();
    return true;
}

bool sonic_init(sonic_sensor_t sensors[], int n_sensors)
{
    if (n_sensors > SONIC_MAX_SENSORS || n_sensors <= 0) return false;
    if (!state.handler_attached) {
        interrupts_attach_handler(process_echo, INTERRUPTS_GPIO3);
        state.handler_attached = true;
    }
    for (int i = 0; i < n_sensors; i++) {
        gpio_set_output(sensors[i].trigger);
        gpio_set_input(sensors[i].echo);
        // Set pulldown b/c `echo` is driven high by sensor
        gpio_set_pulldown(sensors[i].echo);
    }
    state.sensors = malloc(sizeof(sonic_sensor_t) * n_sensors);
    memcpy(state.sensors, sensors, sizeof(sonic_sensor_t) * n_sensors);
    state.curr_data = malloc(sizeof(sonic_data_t) * n_sensors);
    state.arr_readings = sonic_rb_new();
    state.n_sensors = n_sensors;
    state.timeout = SONIC_DEFAULT_TIMEOUT;
    state.unit_delay = SONIC_MIN_DELAY;
    return true;
}

void sonic_deinit(void)
{
    sonic_off();
    free(state.sensors);
    free(state.curr_data);
    sonic_data_t *to_free;
    while (sonic_read_async(&to_free)) free(to_free);
    free((void *)state.arr_readings);
}

int sonic_sensor_count(void)
{
    return state.n_sensors;
}

void sonic_set_unit_delay(unsigned micros)
{
    if (micros >= SONIC_MIN_DELAY) state.unit_delay = micros;
}

void sonic_set_cycle_delay(unsigned micros)
{
    state.cycle_delay = micros;
}

void sonic_set_timeout(unsigned micros)
{
    state.timeout = micros;
}

void sonic_on(void)
{
    state.is_active = true;
    state.curr_sensor = 0;
    countdown_reset(TRIGGER_DELAY);
    countdown_set_mode(COUNTDOWN_MODE_DISCONTINUOUS);
    for (int i = 0; i < state.n_sensors; i++) {
        gpio_enable_event_detection(state.sensors[i].echo, GPIO_DETECT_FALLING_EDGE);
    }
    start_trigger(0); // pc argument is just to match interface; pass dummy value
}

void sonic_off(void)
{
    countdown_disable();
    countdown_disable_interrupts();
    countdown_set_handler(NULL);
    for (int i = 0; i < state.n_sensors; i++) {
        gpio_disable_event_detection(state.sensors[i].echo, GPIO_DETECT_FALLING_EDGE);
    }
    state.is_active = false;
}

bool sonic_is_active(void)
{
    return state.is_active;
}

static inline bool did_timeout(unsigned start, unsigned timeout)
{
    return timer_get_ticks() - start >= timeout;
}

// The `min_valid` is the minimum number of sensors in array
// that must give valid readings (i.e. not time out); otherwise
// the entire reading is considered useless and is redone until
// the criterion is satisfied.
bool sonic_read_sync(sonic_data_t **read_dest, int min_valid)
{
    // If we're reading in async mode already, we can't do both at once
    if (state.is_active) return false;

    sonic_data_t *result = malloc(sizeof(sonic_data_t) * state.n_sensors);
    int valid_readings;
    do {
        valid_readings = state.n_sensors;
        for (int i = 0; i < state.n_sensors; i++) {
            gpio_write(state.sensors[i].trigger, 1);
            timer_delay_us(TRIGGER_DELAY);
            gpio_write(state.sensors[i].trigger, 0);
            unsigned int start = timer_get_ticks();
            while (gpio_read(state.sensors[i].echo) == 0
                && !did_timeout(start, state.timeout)) ;
            while (gpio_read(state.sensors[i].echo) == 1
                && !did_timeout(start, state.timeout)) ;
            unsigned int elapsed = timer_get_ticks() - start;

            if (did_timeout(start, state.timeout)) {
                result[i].distance = SONIC_INVALID_READING;
                result[i].timestamp = start;
                valid_readings--;
                if (valid_readings < min_valid) break;
            } else {
                result[i].distance = (int) ((elapsed * SPEED_MM_MICRO) / 2);
                result[i].timestamp = start + (elapsed / 2);
            }

            timer_delay_us(state.unit_delay);
        }
    } while (valid_readings < min_valid);
    *read_dest = result;
    return true;
}

bool sonic_read_sync_multiple(sonic_data_t *read_dests[], int n_readings, int min_valid)
{
    if (state.is_active) return false;
    for (int i = 0; i < n_readings; i++) {
        // Assume interrupts are off during sync read; no possible way async mode could have
        // been enabled during this function call, so ignore success flag of `sonic_read_sync`
        sonic_read_sync(&read_dests[i], min_valid);
        timer_delay_us(state.cycle_delay);
    }
    return true;
}

bool sonic_read_async(sonic_data_t **read_dest)
{
    return sonic_rb_dequeue(state.arr_readings, read_dest);
}
