#ifndef MOTOR_H
#define MOTOR_H

#define CW 1
#define CCW 0

typedef struct {
    int id;
    int step_pin;
    int dir_pin;
    int direction;
} motor_t;

/*
 * Wire up the motor to the pins you'll be using to drive it.
 */
void motor_init(motor_t motor);

/*
 * Turns all the motors in the array at the speed for each given by the speeds array for the given amount of time.
 * It does this by calculating the steps needed to turn each motor and turning each motor as close to in unison as
 * possible to maintain tension.
 */
void motor_turn_multiple(motor_t motors[], float speeds_rpms[], float time_ms);

/*
 * This function takes in the motor that is being driven, the number of degrees for the motor to be turned, and the 
 * cycle time in microseconds. It turns the motor degrees # of degrees by turning it 1.8 degrees every cycle time 
 * microseconds.
 */
void motor_turn_degrees(motor_t motor, float degrees, int cycle_time_us); 

/*
 * This function turns the motor passed in at speed `speed_rpms` (given in rotations per millisecond) for a given amount 
 * of time passed in in milliseconds.
 */
void motor_turn_speed(motor_t motor, float speed_rpms, float time_ms); 

#endif
