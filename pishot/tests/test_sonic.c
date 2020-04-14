#include "countdown.h"
#include "assert.h"
#include "gpio.h"
#include "interrupts.h"
#include "malloc.h"
#include "printf.h"
#include "sonic.h"
#include <stddef.h> // for NULL
#include "timer.h"
#include "uart.h"

#define N_SENSORS 4

#define N_READINGS_LONG 10000
#define N_READINGS_SHORT 5
#define ASYNC_DELAY_SENSOR 1000
#define ASYNC_DELAY_ARRAY 1000000
void test_async(void)
{
    sonic_set_unit_delay(ASYNC_DELAY_SENSOR);
    sonic_set_cycle_delay(ASYNC_DELAY_ARRAY);

    printf("Testing %d-sensor array for %d cycles.\n", N_SENSORS, N_READINGS_LONG);
    assert(!sonic_is_active());
    sonic_on();
    assert(sonic_is_active());
    sonic_data_t *result;
    int i = 0;
    while (i < N_READINGS_LONG) {
        if (sonic_read_async(&result)) {
            for (int sensor = 0; sensor < N_SENSORS; sensor++) {
                printf("[Reading %d] Sensor %d: Distance = %d mm. Timestamp = %d microsecs.\n",
                i, sensor, result[sensor].distance, (int)result[sensor].timestamp);
                
            }
            printf("\n");
            free(result);
            i++;
        }
    }
    sonic_off();
    assert(!sonic_is_active());

    printf("Finished testing %d-sensor array.\n", N_SENSORS);
    timer_delay(2);
    printf("Testing restart of sonic module for %d cycles.\n", N_READINGS_SHORT);
    sonic_on();
    i = 0;
    while (i < N_READINGS_SHORT) {
        if (sonic_read_async(&result)) {
            for (int sensor = 0; sensor < N_SENSORS; sensor++) {
                printf("[Reading %d] Sensor %d: Distance = %d mm. Timestamp = %d microsecs.\n",
                i, sensor, result[sensor].distance, (int)result[sensor].timestamp);
                
            }
            printf("\n");
            free(result);
            i++;
        }
    }
    sonic_off();
    sonic_deinit();
}

#define SYNC_DELAY_SENSOR 10
#define SYNC_DELAY_ARRAY 100000
#define ITER_DELAY 500000
#define N_ITERS 10000
#define N_READINGS_PER_ITER 2
#define N_TIMEOUTS 4
void test_sync(void)
{
    sonic_set_unit_delay(SYNC_DELAY_SENSOR);
    sonic_set_cycle_delay(SYNC_DELAY_ARRAY);
    printf("Testing %d-sensor array for %d cycles, with max %d timeout(s) per array.\n",
        N_SENSORS, N_READINGS_PER_ITER * N_ITERS, N_TIMEOUTS);
    sonic_data_t *result;
    for (int i = 0; i < N_ITERS; i++) {
        sonic_read_sync(&result, N_READINGS_PER_ITER, N_TIMEOUTS);
        for (int reading = 0; reading < N_READINGS_PER_ITER; reading++) {
            for (int sensor = 0; sensor < N_SENSORS; sensor++) {
                printf("[Reading %d] Sensor %d: Distance = %d mm. Timestamp = %d microsecs.\n",
                    i * N_READINGS_PER_ITER + reading, sensor, result[sensor].distance, (int)result[sensor].timestamp);
            }
            printf("\n");
        }
        free(result);
        timer_delay_us(ITER_DELAY);
    }
}


typedef struct {
     struct gpio_motor {
          unsigned int step;
          unsigned int direction;
     } motors[4];
     sonic_sensor_t sensors[N_SENSORS];
} gpio_layout_t;

static gpio_layout_t get_pin_layout(void)
{
     gpio_layout_t layout;
     layout.motors[0].step = GPIO_PIN3;
     layout.motors[0].direction = GPIO_PIN4;
     layout.motors[1].step = GPIO_PIN10;
     layout.motors[1].direction = GPIO_PIN9;
     layout.motors[2].step = GPIO_PIN25;
     layout.motors[2].direction = GPIO_PIN8;
     layout.motors[3].step = GPIO_PIN5;
     layout.motors[3].direction = GPIO_PIN6;

     layout.sensors[0].echo = GPIO_PIN23;
     layout.sensors[0].trigger = GPIO_PIN24;
     layout.sensors[1].echo = GPIO_PIN17;
     layout.sensors[1].trigger = GPIO_PIN27;
     layout.sensors[2].echo = GPIO_PIN7;
     layout.sensors[2].trigger = GPIO_PIN1;
     layout.sensors[3].echo = GPIO_PIN13;
     layout.sensors[3].trigger = GPIO_PIN19;

     return layout;
}

void main(void) 
{
    interrupts_init();

    gpio_init();
    timer_init();
    uart_init();
    countdown_init(COUNTDOWN_MODE_CONTINUOUS, NULL);
    gpio_layout_t layout = get_pin_layout();
    sonic_init(layout.sensors, N_SENSORS);

    interrupts_global_enable(); // everything fully initialized, now turn on interrupts

    printf("Testing synchronous mode.\n");
    test_sync();
    timer_delay(2);

    printf("Testing asynchronous mode.\n");
    test_async();
    
    printf("Done testing.\n");
}
