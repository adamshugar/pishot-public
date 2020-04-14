#include "countdown.h"
#include "gpio.h"
#include "interrupts.h"
#include "printf.h"
#include "timer.h"
#include "uart.h"

static unsigned end;
static bool countdown_callback(unsigned int pc)
{
    end = timer_get_ticks();
    uart_putstring("Ding!\n");
    return true;
}

static void print_timer_state(void)
{
    printf("ARM timer state: is_enabled: %d, count: %d, load: %d, reload %d.\n",
        countdown_is_enabled(), countdown_get_count(), countdown_get_load(), countdown_get_reload());
}

void main(void) 
{
    interrupts_init();

    gpio_init();
    timer_init();
    uart_init();

    static const int TIME_1 = 1000;
    static const int TIME_2 = 40000;
    static const int TIME_3 = 6969;

    static const int LET_IT_RUN = 100000;

    countdown_init(COUNTDOWN_MODE_DISCONTINUOUS, countdown_callback);
    countdown_enable_interrupts();
    
    interrupts_global_enable(); // everything fully initialized, now turn on interrupts
    
    countdown_set_ticks(TIME_1);
    countdown_enable();
    timer_delay_us(TIME_1 / 2);
    countdown_disable();
    printf("ARM timer disabled after %d micros during %d microsec run.\n", TIME_1 / 2, TIME_1);
    print_timer_state();

    countdown_reset(TIME_2);
    printf("ARM timer reset to %d seconds.\n", TIME_2);
    print_timer_state();
    printf("Enabling...\n");
    unsigned start = timer_get_ticks();
    countdown_enable();
    print_timer_state();
    timer_delay_us(LET_IT_RUN);
    printf("Start: %d, End: %d, Number of microseconds delayed: %d\n", start, end, end - start);
    print_timer_state();

    start = timer_get_ticks();
    countdown_enable();
    timer_delay_us(LET_IT_RUN);
    printf("Start: %d, End: %d, Number of microseconds delayed: %d\n", start, end, end - start);

    start = timer_get_ticks();
    countdown_enable();
    timer_delay_us(LET_IT_RUN);
    printf("Start: %d, End: %d, Number of microseconds delayed: %d\n", start, end, end - start);

    countdown_reset(TIME_3);
    timer_delay_us(LET_IT_RUN);
    start = timer_get_ticks();
    countdown_enable();
    timer_delay_us(LET_IT_RUN);
    printf("Start: %d, End: %d, Number of microseconds delayed: %d\n", start, end, end - start);
}
