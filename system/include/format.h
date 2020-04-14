#ifndef FORMAT_H
#define FORMAT_H

/* 
 * This typedef gives a nickname to the type of function pointer used in
 * printf-style functions. A formatted_fn_t function has one fixed
 * parameter (format string) and variable arguments to follow. The return
 * value is of type int. 
 */
typedef int (*formatted_fn_t)(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

#endif
