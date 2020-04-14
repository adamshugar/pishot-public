#ifndef OBJECT_VECTOR_H
#define OBJECT_VECTOR_H

#include "hoop.h"
#include "sonic.h"

/* 
 * This module uses an array of ultrasonic sensors to detect the position,
 * velocity, and acceleration of an object in 3-D space. The sensors measure
 * distance but since their configuration in 3-D space is known (see following
 * diagram), they can be used to ascertain vector representations of a nearby object.
 *
 *   1              2
 *   * ------------ *
 *   |              |
 *   |              |   +y
 *   |              |   ^
 *   |              |   |          
 *   * ------------ *   |---> +x  (o) +z (out of plane)
 *   4              3
 * 
 * Origin is center of rect. (Internally in module, origin is actually bottom left
 * sensor because that simplifies the math, but the final result is converted to
 * the center-of-rect origin coord system because that is the system used by
 * the motors to drive the hoop.) "*" is a sensor, and the width and height
 * of rectangle are known from the physical setup. These values are hard-coded
 * into constants within this module.
 *
 * The only public function reads from the sensors and makes a prediction
 * as to where the ball will land as quickly as possible. It returns this value
 * to the main loop so the board can move there.
 */

/*
 * Initializes module and registers each element in the `sensors` array as
 * a distinct sonic sensor. Hard-coded to register exactly 4 elements of array
 * in coordination with physical rectangle: element 0 is sensor at top left corner,
 * elem. 1 is top right, elem. 2 is bottom right, and elem. 3 is bottom left.
 */
void object_vector_init(sonic_sensor_t sensors[]);

/*
 * Main work function of module (and only public interface). Reads from the
 * ultrasonic sensor array, determines position of object in 3D space over time,
 * extrapolates the object's trajectory, and predicts where it will land on the
 * board, which is returned via parameter passing.
 * 
 * Returns true if the object will contact the xy-plane at some point
 * in the future based on its current trajectory, false otherwise.
 */
bool object_vector_predict(board_pos_t *prediction);

#endif
