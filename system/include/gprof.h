#ifndef GPROF_H
#define GPROF_H

#include "format.h"
#include <stdbool.h>
#include <stddef.h>

/* Initialize profiler */
void gprof_init(void);

/* Turn on profiling */
void gprof_on(void);

/* Turn off profiling */
void gprof_off(void);

/* Report whether profiling on */
bool gprof_is_active(void);

/* Print the profiler results */
void gprof_dump(formatted_fn_t print_fn, const char *label, size_t n_highest);

/* Return number of instructions in current program */
size_t gprof_get_instruction_count(void);

#endif
