#include "countdown.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Originally written by Julie Zelenski. Adapted significantly by Adam Shugar
 * on March 12, 2020.
 */

/*
 * See BCM p 196 (unfortunately the documentation is wrong in several places)
 *
 * This timer has two parts: a free running counter and a count-down timer.
 * The timer is always running in free running mode.
 *
 * In this module, we implement the count-down timer. Interval is set as
 * load/re-load, will count down from there to zero. At zero, will report
 * overflow and generate interrupt (if interrupts enabled)
 *
 * The timer and counter are based on the APB clock, which on the
 * Raspberry Pi is 250 Mhz
 * 
 * Some errors in the documentation:
 *  (1) the counters are either 32-bit (not 23-bit) or 16-bit
 *  (2) the effect of pre-scale bits (3:2) in the control register is unclear.
 */

struct countdown_state {
    bool is_discontinuous;
    countdown_handler_t handler;
};
static struct countdown_state state;
static bool countdown_done(unsigned pc)
{
    if (state.handler == NULL) return false;
    if (state.is_discontinuous) countdown_disable();
    countdown_check_and_clear_interrupt();
    state.handler();
    return true;
}

struct countdown_t {
    uint32_t load;                  // writing immediately loads the counter
    uint32_t value;                 // current counter value, read only
    struct {
        uint32_t :1,
        timer_is_32_bit:1,          // 0 -> 16-bit, 1 -> 32 bit
        :3,
        enable_timer_interrupt:1,   // 1 = timer interrupts enabled
        :1,
        enable_timer:1,             // 1 - timer enabled
        run_in_debug:1,             // 1 = run timer in arm debug mode/halt
        free_counter_enable:1,      // 1 = free running counter enabled
        :6,
        free_counter_prescale:8;    // prescalar for the free running counter
                                    // clk/(prescale+1)
    } control;
    uint32_t clear_event;           // clear overflow/interrupt, write only
    uint32_t overflow;              // overflow/interrupt pending, read only
    uint32_t irq;                   // pending & interrupt enabled, read only
    uint32_t reload;                // value to be reloaded when value hits 0
    uint32_t prescale;              // prescaler for countdown timer clk/(prescale+1)
    uint32_t free_counter_value;    // free running counter value
};

#define ARMTIMER_BASE 0x2000B400;

static volatile struct countdown_t * const countdown = (struct countdown_t *)ARMTIMER_BASE;


/* Cute hack: compile-time assertion . */
#define AssertNow(x) switch(1) { case (x): case 0: ; }

/*
 * initialize countdown
 *
 * ticks is the number of usecs between overflows/interrupts
 *
 * timer is initialized, but not enabled. interrupts are disabled.
 * 
 */
void countdown_init(countdown_mode_t mode, countdown_handler_t handler) 
{ 
    // make sure bit-fields are within 1 word
    AssertNow(sizeof countdown->control == 4);

    state.is_discontinuous = mode == COUNTDOWN_MODE_DISCONTINUOUS;
    state.handler = state.is_discontinuous ? handler : NULL;
    interrupts_attach_handler(countdown_done, INTERRUPTS_BASIC_ARM_TIMER_IRQ);

    countdown->control.enable_timer = 0;
   
    countdown->control.enable_timer_interrupt = state.is_discontinuous; 
    countdown->clear_event = 0;
    countdown->control.timer_is_32_bit = 1;

    // setup up timer it counts once per microsecond
    countdown->prescale = (250-1); // 250 000 000 / 250 = 1 000 000 = 1 usec

    // ticks are number of microseconds between overflows/interrupts
    // initialize at max value and leave user to set value later
    countdown->load = -1;    // load value is immediately loaded into the counter
    countdown->reload = -1;  // reload value is loaded into the counter when it hits 0
}

/*
 * set timer prescalar (apbclock/divisor)
 *
 * the timer counts at a rate equal to the ABP clock divided by the divisor.
 * the APB clock on the Raspberry Pi runs at 250 Mhz.  thus, if the divisor 
 * equals 250, the timer will count once per usec.
 *
 * The prescale register is 10-bits
 *
 * The reset value of this register is 0x7D so gives a divide by 126.
 */
__attribute__((unused)) static void countdown_set_prescalar(unsigned int divisor)
{
    countdown->prescale = divisor-1;
}

void countdown_enable(void)
{
    countdown->control.enable_timer = 1;
}

void countdown_disable(void)
{
    countdown->control.enable_timer = 0;
}

bool countdown_is_enabled(void)
{
    return countdown->control.enable_timer;
}

void countdown_enable_interrupts(void)
{
    countdown->control.enable_timer_interrupt = 1;
}

void countdown_disable_interrupts(void)
{
    countdown->control.enable_timer_interrupt = 0;
}

unsigned int countdown_get_count(void)
{
    return countdown->value;
}

unsigned int countdown_get_load(void)
{
    return countdown->load + 1;
}

unsigned int countdown_get_reload(void)
{
    return countdown->reload + 1;
}

void countdown_set_ticks(unsigned int ticks)
{
    countdown->reload = ticks - 1;
    countdown->load = ticks - 1;
}

void countdown_reset(unsigned int ticks)
{
    countdown_disable();
    countdown_set_ticks(ticks);
}

bool countdown_check_overflow(void)
{
    return countdown->overflow != 0;
}

void countdown_clear_event(void)
{
    countdown->clear_event = 1;
}

bool countdown_check_and_clear_overflow(void)
{
    bool had_event = countdown_check_overflow();
    if (had_event) countdown_clear_event();
    return had_event;
}

bool countdown_check_interrupt(void)
{
    return countdown->irq != 0;
}

bool countdown_check_and_clear_interrupt(void)
{
    bool had_event = countdown_check_interrupt();
    if (had_event) countdown_clear_event();
    return had_event;
}

countdown_mode_t countdown_get_mode(void)
{
    return state.is_discontinuous ? COUNTDOWN_MODE_DISCONTINUOUS : COUNTDOWN_MODE_CONTINUOUS;
}

void countdown_set_mode(countdown_mode_t mode)
{
    state.is_discontinuous = mode == COUNTDOWN_MODE_DISCONTINUOUS;
    // Automatically enable interrupts if timer is set up with interrupt handlers
    if (state.is_discontinuous) countdown_enable_interrupts();
}

void countdown_set_handler(countdown_handler_t handler)
{
    state.handler = handler;
}
