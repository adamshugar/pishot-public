#ifndef COUNTDOWN_H
#define COUNTDOWN_H

#include "interrupts.h"
#include <stdbool.h>

/*
 * Functions for optionally interrupt-based, optionally continuous
 * countdown clock implemented using ARM timer. Adapted from code
 * written by these wonderful folks:
 * 
 * Pat Hanrahan <hanrahan@cs.stanford.edu>
 * Dawson Engler <engler@cs.stanford.edu>
 * Julie Zelenski <zelenski@cs.stanford.edu>
 *
 * Written by Adam Shugar for CS107e.
 */ 

// Determines whether, after hitting 0, the timer starts counting
// again automatically (continuous) or stops itself (discontinuous)
enum countdown_mode {
    COUNTDOWN_MODE_CONTINUOUS,
    COUNTDOWN_MODE_DISCONTINUOUS,
};
typedef enum countdown_mode countdown_mode_t;

/*
 * Initialize the armtimer peripheral which provides a
 * "countdown" timer.
 * 
 * The `mode` parameter specifies whether the
 * timer should stop after finished counting down (`COUTNDOWN_MODE_DISCONTINUOUS`)
 * or whether it should immediately start another countdown cycle
 * (`COUNTDOWN_MODE_CONTINUOUS`).
 * 
 * The `handler` parameter specifies which function, if any,
 * should be called when the timer finishes counting down.
 * If this argument is null, no function will be called.
 *
 * The timer does not automatically turn on after init.
 * The default countdown time is the maximum possible until
 * otherwise set by the client.
 *
 * NOTE: For a handler and/or discontinuous mode to work,
 * interrupts must be enabled.
 *
 * When the countdown hits zero, an overflow event
 * happens. If interrupts are enabled, it will also generate an 
 * interrupt on that event. At point of overflow/interrupt,
 * timer will restart counting down if in continuous mode.
 */
void countdown_init(countdown_mode_t mode, handler_fn_t handler);

/*
 * Enable the countdown timer. Starts counting down.
 */
void countdown_enable(void);

/*
 * Disable the countdown timer. Suspends counting down.
 */
void countdown_disable(void);

/*
 * Returns true if the timer is currently counting down, false otherwise.
 */
bool countdown_is_enabled(void);

/*
 * If interrupts are enabled, an interrupt is generated when the 
 * countdown timer reaches zero.
 */
void countdown_enable_interrupts(void);

/*
 * If interrupts are disabled, no interrupt is generated when the 
 * countdown timer reaches zero.
 */
void countdown_disable_interrupts(void);

/*
 * Returns the current value of the countdown timer. 
 */
unsigned int countdown_get_count(void);

/*
 * Returns the currently set tick max from which to count down.
 */
unsigned int countdown_get_load(void);

/*
 * Returns the tick max from which to count down
 * starting with the next cycle.
 */
unsigned int countdown_get_reload(void);

/*
 * Immediately sets the current number of ticks until 0 to `ticks`.
 */
void countdown_set_ticks(unsigned int ticks);

/*
 * Pauses the timer and sets the current number of ticks until 0 to `ticks`.
 */
void countdown_reset(unsigned int ticks);

/* 
 * Returns whether an overflow event has occurred (i.e. countdown
 * has reached zero). The return value is true if overflow event 
 * has occurred and the event has not been cleared, false otherwise.
 */
bool countdown_check_overflow(void);

/*
 * Clears event status. Any previous overflow event is cleared.
 */
void countdown_clear_event(void);

/* 
 * Returns whether an overflow event occurred and clears the
 * event.
 */
bool countdown_check_and_clear_overflow(void);

/*
 * Returns whether an interrupt was generated (i.e. overflow 
 * event has occurred and interrupts are enabled).
 * The return value is true if interrupt has occurred and 
 * the event has not been cleared, false otherwise.
 */
bool countdown_check_interrupt(void);

/*
 * Returns whether an interrupt was generated and clears the
 * event.
 */
bool countdown_check_and_clear_interrupt(void);

/*
 * Returns the current operating mode of the timer:
 * COUNTDOWN_MODE_DISCONTINUOUS if discontinuous, or
 * COUNTDOWN_MODE_CONTINUOUS if continuous.
 */
countdown_mode_t countdown_get_mode(void);

/*
 * Sets the current operating mode of the timer to `mode`.
 */
void countdown_set_mode(countdown_mode_t mode);

/*
 * Sets the handler called when the timer hits 0 to `handler`.
 */
void countdown_set_handler(handler_fn_t handler);

#endif
