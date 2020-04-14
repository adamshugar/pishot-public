#ifndef HOOP_H
#define HOOP_H

#include "motor.h"

typedef struct {
    unsigned int step_pin;
    unsigned int dir_pin;
} motor_init_t;

typedef struct {
    float x;
    float y;
} board_pos_t;

// In millimeters; max distance in + direction or - direction that motors
// can move the hoop
#define HOOP_BOUND_WIDTH 100
#define HOOP_BOUND_HEIGHT 100

/*
 * Sets up the four motors used in this project to the correct GPIO pins and prepares the array of motors for later use
 */
void hoop_init(motor_init_t motors_init[]);

/* 
 * Moves the hoop from current location to `destination` (constrained by permissible bounds of motion)
 * by moving each motor to its new position as fast as possible.
 */
void hoop_move(board_pos_t destination); 

#endif
