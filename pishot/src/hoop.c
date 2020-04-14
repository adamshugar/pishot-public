#include "gpio.h"
#include "hoop.h"
#include "motor.h"
#include "timer.h"
#include "utils.h"

/*
 * Written by Ryan Johnston on March 13, 2020.
 */

#define N_MOTORS 4
#define MOTOR_DIAMETER 23 // Millimeters
#define PI 3.1415
// Offsets of motors from origin in millimeters
#define X1 -560
#define Y1 550
#define X2 560
#define Y2 550
#define X3 -560
#define Y3 -550
#define X4 560
#define Y4 -550

#define MAX_SPEED 0.0016 // Rotations per ms

#define NUM_STEPS 20

static motor_t motors[N_MOTORS];
static board_pos_t cur; // Current hoop position

// Determines order to pass motor pins to motor_init()
enum {
    MOTOR_TOP_LEFT = 0,
    MOTOR_TOP_RIGHT,
    MOTOR_BOTTOM_LEFT,
    MOTOR_BOTTOM_RIGHT,
};

// Works for exactly 4 motors
void hoop_init(motor_init_t motors_init[]) {
    // Assume hoop starts at center bottom
    cur.x = 0;
    cur.y = Y3;

    for (int i = 0; i < N_MOTORS; i++) {
        motors[i].id = i;
        motors[i].step_pin = motors_init[i].step_pin;
        motors[i].dir_pin = motors_init[i].dir_pin;
        motor_init(motors[i]);
    }
}

static float get_delta(motor_t motor, float x1, float y1, float x2, float y2) {
    float mx = 0;
    float my = 0;
    switch(motor.id) {
        case MOTOR_TOP_LEFT: mx = X1, my = Y1;
        break;
        case MOTOR_TOP_RIGHT: mx = X2, my = Y2;
        break; 
        case MOTOR_BOTTOM_LEFT: mx = X3, my = Y3;
        break; 
        case MOTOR_BOTTOM_RIGHT: mx = X4, my = Y4;
        break;
    }
    float z1 = sqrt((x1 - mx)*(x1 - mx) + (y1 - my)*(y1 - my));
    float z2 = sqrt((x2 - mx)*(x2 - mx) + (y2 - my)*(y2 - my));
    float delta = z2 - z1;
    if ((delta < 0 && motor.id % 2 == 0) || (delta >= 0 && motor.id % 2 == 1)) {
        motors[motor.id].direction = CCW; 
    } else {
        motors[motor.id].direction = CW;
    }
    if (delta < 0) delta = -delta;
    return delta;
}
 
static float get_time(board_pos_t destination) {
    destination.x = min(HOOP_BOUND_WIDTH, max(-HOOP_BOUND_WIDTH, destination.x));
    destination.y = min(HOOP_BOUND_HEIGHT, max(-HOOP_BOUND_HEIGHT, destination.y));
    float deltas[N_MOTORS];
    for (int i = 0; i < N_MOTORS; i++) {
        deltas[i] = get_delta(motors[i], cur.x, cur.y, destination.x, destination.y);
    }
    float max_delta = deltas[0];
    for (int i = 0; i < N_MOTORS; i++) {
        if (deltas[i] > max_delta) max_delta = deltas[i];
    }
    float time = 100; // In milliseconds
    float max_speed = max_delta / (MOTOR_DIAMETER * PI) / time;
    while (max_speed > MAX_SPEED) {
        time += 100;
        max_speed = max_delta / (MOTOR_DIAMETER * PI) / time;
    }
    return time;
}

void hoop_move(board_pos_t destination) {
    float time_step = get_time(destination) / NUM_STEPS;
    float x_step = (destination.x - cur.x) / NUM_STEPS;
    float y_step = (destination.y - cur.y) / NUM_STEPS;
    float new_x = cur.x + x_step;
    float new_y = cur.y + y_step;
    for (int i = 0; i < NUM_STEPS; i++) {
        float deltas[N_MOTORS];
        for (int i = 0; i < N_MOTORS; i++) {
            deltas[i] = get_delta(motors[i], cur.x, cur.y, new_x, new_y);
        }
        float speeds[N_MOTORS];
        for (int i = 0; i < N_MOTORS; i++) {
            speeds[i] = deltas[i] / (MOTOR_DIAMETER * PI) / time_step;
        }
        motor_turn_multiple(motors, speeds, time_step);
        cur.x = new_x;
        cur.y = new_y;
        new_x += x_step;
        new_y += y_step;
    }
}
