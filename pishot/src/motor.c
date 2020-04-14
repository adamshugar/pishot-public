#include "gpio.h"
#include "timer.h"
#include "motor.h"

/*
 * Written by Ryan Johnston on March 9, 2020.
 */

#define STEP_ANGLE 1.8 // In degrees

void motor_init(motor_t motor) {
    gpio_set_output(motor.step_pin);
    gpio_set_output(motor.dir_pin);
}

void motor_turn_multiple(motor_t motors[], float speeds_rpms[], float time_ms) {
    for (int i = 0; i < 4; i++) {
        gpio_write(motors[i].dir_pin, motors[i].direction);
    }
    int steps[4];
    for (int i = 0; i < 4; i++) {
        steps[i] = (int)((speeds_rpms[i] * time_ms * 360) / STEP_ANGLE);
    }
    int max_steps = steps[0];
    for (int i = 0; i < 4; i++) {
        if (steps[i] > max_steps) max_steps = steps[i];
    }
    int skips[4];
    for (int i = 0; i < 4; i++) {
        if (steps[i] < max_steps) {
            if (steps[i] * 2 > max_steps) skips[i] = steps[i] / (max_steps - steps[i]) + 1;
            else  skips[i] = -((max_steps - steps[i]) / steps[i] + 1); 
        } else skips[i] = max_steps + 1;
    }
    int cycle_time_us = (int)(time_ms * 1000 / max_steps / 2);
    for (int i = 0; i < max_steps; i++) {
        for (int j = 0; j < 4; j++) {
            if ((skips[j] < 0 && ((i + 1) % -skips[j] == 0)) || (skips[j] > 0 && ((i + 1) % skips[j] != 0))) {
                gpio_write(motors[j].step_pin, 1); 
                timer_delay_us(cycle_time_us);
                gpio_write(motors[j].step_pin, 0);
                timer_delay_us(cycle_time_us);
            }
        }
    }
}

void motor_turn_degrees(motor_t motor, float degrees, int cycle_time_us) {
    gpio_write(motor.dir_pin, motor.direction);
    int steps = (int)(degrees / STEP_ANGLE);
    for (int i = 0; i < steps; i++) {
        gpio_write(motor.step_pin, 1);
        timer_delay_us(cycle_time_us);
        gpio_write(motor.step_pin, 0);
        timer_delay_us(cycle_time_us);
    }
}

void motor_turn_speed(motor_t motor, float speed_rpms, float time_ms) {
    float degrees = speed_rpms * time_ms * 360;
    float steps = time_ms * speed_rpms * 360 / STEP_ANGLE;
    int cycle_time_us = (int)(time_ms * 1000 / steps / 2);
    motor_turn_degrees(motor, degrees, cycle_time_us);
}
