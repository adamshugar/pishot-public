/*
 * Defines various utility functions, including:
 * - min:    returns minimum of two values.
 * - max:    returns maximum of two values.
 * - abs:    returns absolute value of an int.
 * - square: multiplies value by itself once.
 * - swap:   swaps values of two integers passed by reference.
 * - round:  rounds a float to nearest int value.
 * - sqrt:   returns sqrt of float value (-1 if f < 0).
 */
#ifndef UTILS_H
#define UTILS_H

#define min(X, Y) ((X) < (Y) ? (X) : (Y))
#define max(X, Y) ((X) > (Y) ? (X) : (Y))
#define abs(X)    ((X) < 0 ? -(X) : (X))
#define square(X) ((X) * (X))

void swap(int *a, int *b);

int round(float f);

float sqrt(float f);

#endif