#ifndef SONIC_H
#define SONIC_H

#include <stdbool.h>
#include <stddef.h> // for size_t

/* 
 * NOTE: For asynchronous mode, client must first initialize countdown
 * module from main program.
 *
 * Interface for HC-SR04 sonic time-of-flight distance sensor.
 * Empirically-tested maximum distance for sensor is about 280 cm,
 * although the specification claims 400 cm.
 * 
 * This module maintains a list of the currently active sonic sensors
 * and continually reads from each back-to-back in the order they were
 * added during initialization. The rationale for this design is that
 * multiple sonic sensors in proximity will interfere with one another
 * if they all emit signals simultaneously, so the array of nearby sensors
 * must be read element-wise in rapid succession.
 * 
 * IMPORTANT: This module utilizes the interrupts-based armtimer. The client
 * should not change the state of the armtimer at all while this module is
 * running (i.e. `sonic_on()` has been called).
 *
 * Written by Adam Shugar for PiShot in CS 107e, Winter 2020
 */
typedef struct {
    int distance;  // In millimeters (accurate to +/-3mm); -1 if no object detected
    int timestamp; // Pi timer reading halfway between trigger and echo
                   // (theoretically this is the timestamp of the exact
                   // moment the sound pulse reflected off the object).
                   // If no object detected, timestamp is set to timer
                   // reading immediately after trigger.
} sonic_data_t;

typedef struct {
    unsigned trigger;   // GPIO pin to trigger sensor to send sonic pulse
    unsigned echo;      // GPIO pin to read when pulse is received back
} sonic_sensor_t;

// Maximum number of simultaneous sonic sensors supported by this module
#define SONIC_MAX_SENSORS 10
// Minimum allowable delay between adjacent sensor readings, in microseconds
#define SONIC_MIN_DELAY 1
// Value for sensors that don't detect an object in range
#define SONIC_INVALID_READING -1

/*
 * Initializes module and registers each element in the `sensors` array as
 * a distinct sonic sensor. Returns true if the initialization was successful,
 * false otherwise. Initialization fails if `n_sensors` exceeds the maximum allowed number.
 */
bool sonic_init(sonic_sensor_t sensors[], size_t n_sensors);

/*
 * Returns the number of currently registered sensors. Guaranteed to be at least
 * 0 and less than or equal to the maximum number of sensors supported.
 */
int sonic_sensor_count(void);

/*
 * Instructs the module to insert a `micros` microsecond delay between each
 * sensor reading, including a between-cycle delay if current cycle delay less
 * than the minimum allowable delay. Otherwise, cycle delay overrides between cycles.
 */
void sonic_set_unit_delay(unsigned micros);

/*
 * Instructs the module to insert a `micros` microsecond delay between each reading
 * of the final sensor and the reading of the first sensor in the following cycle.
 * Default is same as minimum unit delay (`SONIC_MIN_DELAY`).
 */
void sonic_set_cycle_delay(unsigned micros);

/*
 * Initiates continuous interrupts-based reading from all registered sensors.
 * Reading is done starting with first sensor registered and once the last
 * sensor is read, the cycle immediately starts again. If any sensor takes
 * longer than the "timeout" number of microseconds to echo, it is skipped
 * on the current iteration and its distance reading is set to
 * `SONIC_INVALID_READING`.
 */
void sonic_on(void);

/*
 * Turns off continuous interrupts-based reading but does NOT clear buffer of
 * previous reads.
 */
void sonic_off(void);

/*
 * Returns true if module is on, i.e. currently reading data from sensor(s),
 * false otherwise.
 */
bool sonic_is_active(void);

/*
 * Frees up all memory and saved readings associated with this module and clears
 * the state. After calling this function it is as if the module were never init'ed.
 */
void sonic_deinit(void);

// In microseconds; (3 m max dist) / (343 m / s sound in air) = 0.017492 s
#define SONIC_DEFAULT_TIMEOUT 17492
/*
 * Sets the maximum time period to wait while reading from a single sensor
 * on a given iteration. Default timeout is time needed to detect object
 * 300 cm away from the sensor, roughly 10% farther than the furthest reliable
 * reading obtained from testing the sensor. Takes effect starting with the next
 * sensor reading. Overrides the default timeout.
 */
void sonic_set_timeout(unsigned micros);

/*
 * Returns (by parameter passing) heap-allocated array of `readings` sensor array
 * readings, each with maximum `timeout_threshold` invalid readings. Returns true
 * if reading succeeded, otherwise (if async is currently on).
 *
 * It is the client's responsbility to free the resulting data once finished.
 */
bool sonic_read_sync(sonic_data_t **read_dest, size_t readings, size_t timeout_threshold);

/*
 * Returns (by parameter passing) a dynamically-allocated array of distance
 * readings with one element for each registered sensor. It is the client's
 * responsibility to know the array size and to free this data once it is 
 * no longer needed. If there is no data to report, the parameter is unchanged.
 * Assumes that the parameter is a valid memory address to write to.
 * Returns true if data was written, false otherwse.

 * NOTE: Does NOT trigger any gathering of new data from sensors;
 * only dequeues previously read data.
 */
bool sonic_read_async(sonic_data_t **read_dest);

#endif
