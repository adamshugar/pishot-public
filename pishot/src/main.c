#include "gpio.h"
#include "hoop.h"
#include "interrupts.h"
#include "malloc.h"
#include "object_vector.h"
#include "printf.h"
#include "sonic.h"
#include "timer.h"

/*
 * Written by Adam Shugar on March 11, 2020.
 */

/*
 * Ideas for additions to the project:
 * - three separate modes: sink = hoop goes toward ball; avoid = hoop avoids ball;
 *   arrowkeys = second player controls hoop via arrowkeys
 * - run from console and return to console after done (with uart-non ps2 console)
 * - 3d display of position, velocity and acceleration of ball, parameterized by time
 * - flash finished product to SD card entirely on its own without booting up to computer
 */

// TODO: Update makefile for pishot and system library to -Ofast once development is finished

#define N_MOTORS 4
#define N_SENSORS 4

typedef struct {
     motor_init_t motors[N_MOTORS];
     sonic_sensor_t sensors[N_SENSORS];
} gpio_layout_t;

static gpio_layout_t get_pin_layout(void)
{
     // NOTE: Sensor layout is:
     // 0 ---- 1
     // |      |
     // |      |
     // 2 ---- 3
     gpio_layout_t layout;
     layout.motors[0].step_pin = GPIO_PIN2;
     layout.motors[0].dir_pin = GPIO_PIN3;
     layout.motors[1].step_pin = GPIO_PIN10;
     layout.motors[1].dir_pin = GPIO_PIN9;
     layout.motors[2].step_pin = GPIO_PIN25;
     layout.motors[2].dir_pin = GPIO_PIN8;
     layout.motors[3].step_pin = GPIO_PIN5;
     layout.motors[3].dir_pin = GPIO_PIN6;

     // NOTE: Sensor layout is:
     // 0 ---- 1
     // |      |
     // |      |
     // 3 ---- 2
     layout.sensors[0].echo = GPIO_PIN23;
     layout.sensors[0].trigger = GPIO_PIN24;
     layout.sensors[1].echo = GPIO_PIN17;
     layout.sensors[1].trigger = GPIO_PIN27;
     layout.sensors[2].echo = GPIO_PIN13;
     layout.sensors[2].trigger = GPIO_PIN19;
     layout.sensors[3].echo = GPIO_PIN7;
     layout.sensors[3].trigger = GPIO_PIN1;

     return layout;
}

void main(void) 
{
     interrupts_init();

     gpio_layout_t layout = get_pin_layout();
     hoop_init(layout.motors);
     object_vector_init(layout.sensors);

     while (true) {
          board_pos_t ball_hit;
          if (object_vector_predict(&ball_hit)) hoop_move(ball_hit);
     }
}
