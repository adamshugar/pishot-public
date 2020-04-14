#include "motor.h"
#include "hoop.h"
#include "gpio.h"

void basic_test(void) {
     motor_t motor1;
     motor1.id = 1;
     motor1.step_pin = GPIO_PIN2;
     motor1.dir_pin = GPIO_PIN3;
     motor_init(motor1);
     motor_turn_degrees(motor1, 360*4, 500);
    
     motor_t motor2;
     motor2.id = 2;
     motor2.step_pin = GPIO_PIN10;
     motor2.dir_pin = GPIO_PIN9;
     motor_init(motor2);
     motor_turn_degrees(motor2, 360*4, 500);

     motor_t motor3;
     motor3.id = 3;
     motor3.step_pin = GPIO_PIN8;
     motor3.dir_pin = GPIO_PIN7;
     motor_init(motor3); 
     motor_turn_degrees(motor3, 360*4, 500);  
 
     motor_t motor4;
     motor4.id = 4;
     motor4.step_pin = GPIO_PIN5;
     motor4.dir_pin = GPIO_PIN6;
     motor_init(motor4);
     motor_turn_degrees(motor4, 360*4, 500);
}

void test_multiple() {
     motor_t motors[4];

     motor_t motor1;
     motor1.id = 1;
     motor1.step_pin = GPIO_PIN2;
     motor1.dir_pin = GPIO_PIN3;
     motor_init(motor1);
     motors[0] = motor1;
    
     motor_t motor2;
     motor2.id = 2;
     motor2.step_pin = GPIO_PIN10;
     motor2.dir_pin = GPIO_PIN9;
     motor_init(motor2);
     motors[1] = motor2;

     motor_t motor3;
     motor3.id = 3;
     motor3.step_pin = GPIO_PIN8;
     motor3.dir_pin = GPIO_PIN7;
     motor_init(motor3);
     motors[2] = motor3;  
 
     motor_t motor4;
     motor4.id = 4;
     motor4.step_pin = GPIO_PIN5;
     motor4.dir_pin = GPIO_PIN6;
     motor_init(motor4);
     motors[3] = motor4;
     
     float speeds[4] = {0.0015, 0.002, 0.003, 0.004};
  
     motor_turn_multiple(motors, speeds, 2000);
}

void test_max_speed(void) { 
     motor_t motors[4];

     motor_t motor1;
     motor1.id = 1;
     motor1.step_pin = GPIO_PIN2;
     motor1.dir_pin = GPIO_PIN3;
     motor_init(motor1);
     motors[0] = motor1;
    
     motor_t motor2;
     motor2.id = 2;
     motor2.step_pin = GPIO_PIN10;
     motor2.dir_pin = GPIO_PIN9;
     motor_init(motor2);
     motors[1] = motor2;

     motor_t motor3;
     motor3.id = 3;
     motor3.step_pin = GPIO_PIN8;
     motor3.dir_pin = GPIO_PIN7;
     motor_init(motor3);
     motors[2] = motor3;  
 
     motor_t motor4;
     motor4.id = 4;
     motor4.step_pin = GPIO_PIN5;
     motor4.dir_pin = GPIO_PIN6;
     motor_init(motor4);
     motors[3] = motor4;
     for (int j = 1; j < 20; j++) {
         float speeds[4];
         for (int i = 0; i < 4; i++) speeds[i] = 0.001 * j;
         motor_turn_multiple(motors, speeds, 1000);
     }
}

void test_move_hoop_clean(void) {
     hoop_move((board_pos_t){ .x = 100, .y = 100});
}

void move_to_start(void) {
     motor_init_t motors[4];
     motor_init_t motor1;
     motor1.step_pin = GPIO_PIN2;
     motor1.dir_pin = GPIO_PIN3;
     motors[0] = motor1;
    
     motor_init_t motor2;
     motor2.step_pin = GPIO_PIN10;
     motor2.dir_pin = GPIO_PIN9;
     motors[1] = motor2;

     motor_init_t motor3;
     motor3.step_pin = GPIO_PIN8;
     motor3.dir_pin = GPIO_PIN7;
     motors[2] = motor3;  
 
     motor_init_t motor4;
     motor4.step_pin = GPIO_PIN5;
     motor4.dir_pin = GPIO_PIN6;
     motors[3] = motor4;
     hoop_init(motors);
     hoop_move((board_pos_t){ .x = 0, .y = 0});
}

void main(void) {
     move_to_start();
     test_move_hoop_clean();
}
