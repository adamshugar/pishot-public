#include "malloc.h"
#include "object_vector.h"
#include "utils.h"

/*
 * Written by Adam Shugar on March 12, 2020.
 */

#define N_SENSORS 4

void object_vector_init(sonic_sensor_t sensors[])
{
    sonic_init(sensors, N_SENSORS);
}

/*
 * This module works as follows: first, get some number of valid
 * readings from the ultrasonic sensor array (where valid means
 * that at least 3 sensors read a true distance, since 3 spheres
 * are needed to calculate a 3D point). Then, use these distances 
 * and our pre-existing knowledge of the 3D layout of the sensors
 * to determine a 3D position vector R for the object/ball, with
 * respect to the plane of the board.
 * 
 * Do this multiple times in rapid succession over time intervals
 * of known length, and use the adjacent position vector values
 * to calculate a velocity and acceleration vector for the object.
 * Since we now have comprehensive 3D kinematic information for the
 * object, use this to predict where/whether the object will hit
 * the hoop board. At the top level of the module, send this
 * computed information to the hoop moving module to move the hoop
 * to the predicted location.
 */

/*
 * NOTE: Back of the envelope calculation ahead. What is the maximum
 * number of sensor array readings we want to use to calculate a
 * single guess for the ball? The fundamental tradeoff is between
 * how many guesses we can make and the accuracy of an individual
 * guess. We are shooting for ~80% accuracy (nothing spectacular)
 * and a high guess frequency rather than one 95% accurate guess,
 * since an iterative approach provides more opportunity to correct
 * for errors that emerge.
 *
 * A single 4 sensor array reading takes 6 milliseconds. Processing
 * this data to send to the hoop will take maximum about 5 milliseconds.
 * We assume that moving the motors takes 50 milliseconds.
 * Therefore, to make at least 5 guesses within 500 milliseconds, we
 * don't want to go above n = 7 for this module.
 *
 * We also don't need to worry about the noise introduced by the ball
 * moving during a single array reading (assuming n is reasonably small).
 * Max ball speed = 15 ft/sec = 4.5 m/s = 4.5 mm / ms = ~3 cm for one
 * 6 ms single-array reading.
 */

// NOTE: All spatial quantities are in millimeters and all vels/accels are in mm/s(^2)

typedef struct {
    float x;
    float y;
    float z;
} vec_3d_t;


// --------------- BEGIN 3D POSITION MODULE ---------------
#define RECT_WIDTH 1219 // in mm
#define RECT_HEIGHT 1219 // in mm

typedef struct {
    float displacement; // of center of circle from origin; whether displacement is for x or y
                        // component is known by internal calling functions using this data type
    float radius;
} circle_t;

#define NO_INTERSECTION -1
// Function computing the circle (if any) created by the intersection of two spheres. This
// is applied to the spheres of the sensor array to determine the object's (x, y) position.
// `r_onzero`: the radius of the spheres w/ center on one or both coordinate axes (contrast with
// `r_offzero`, the radius of the spheres w/ center on strictly fewer, i.e. zero or one, coordinate axes).
// `centers_dist`: distance between center points of two spheres.
// https://mathworld.wolfram.com/Sphere-SphereIntersection.html
static circle_t xy_sphere_intersect(float r_offzero, float r_onzero, float centers_dist)
{
     float displacement, radius;
     if (r_onzero + r_offzero >= centers_dist) { // Spheres intersect
          // Formula for sphere intersection
          displacement = (square(centers_dist) - square(r_offzero) + square(r_onzero)) / (2 * centers_dist);
          radius = sqrt(square(r_onzero) - square(displacement));
          if (radius < 0) { // One sphere completely encloses the other; set radius to -1 to signal no intersection
               radius = -1;
               displacement = 0;
          }
     } else { // Spheres don't intersect (too small); return halfway point between their closest bounds
          displacement = ((centers_dist - r_offzero) + (r_onzero)) / 2;
          radius = 0;
     }
     return (circle_t) { .displacement = displacement, .radius = radius };
}

// `offset_perpendic` is distance of center of circle away from center of sphere in direction perpendicular
// to plane of sphere (similar meaning for `offset_parallel`).
// Assumes sphere and circle are NOT concentric.
static bool sphere_circle_hit(float r_sphere, float r_circle, float offset_perpedic, float offset_parallel)
{
    if (abs(offset_perpedic) > r_sphere) return false;
    float parallel_dist = abs(offset_parallel);
    // One edge of circle must be on one side (interior or exterior) of sphere, while other circle edge
    // must be on other side of sphere
    return (parallel_dist - r_circle < r_sphere) && (parallel_dist + r_circle > r_sphere);
}

// Similar to sphere intersection function, but all we care about is the z-coordinate
// (depth) of the intersection point between the circle and sphere. We use this
// geometry to compute the height of the object above the board.
// Assumes sphere and circle are NOT concentric.
// https://mathworld.wolfram.com/Circle-CircleIntersection.html
static float z_circle_sphere_intersect(float r_sphere, float r_circle, float offset_perpendic, float offset_parallel)
{
    // First, find the circular cross-section within the sphere that the actual circle intersects.
    float r_circ_intersec = r_sphere - offset_perpendic;
    // Now just compute height of intersection of two circles, easy-peasy
    float intersec_dist_xyplane = (square(offset_parallel) - square(r_circ_intersec) + square(r_circle)) / (2 * offset_parallel);
    float z = sqrt(square(r_circle) - square(intersec_dist_xyplane));
    return z < 0 ? NO_INTERSECTION : z;
}

// Determined by empirically testing sensors (HC-SR04 ultrasonics)
#define MAX_SENSE_DEPTH 3000 // in mm
enum {
    SENSOR_TOP_LEFT = 0,
    SENSOR_TOP_RIGHT,
    SENSOR_BOTTOM_RIGHT,
    SENSOR_BOTTOM_LEFT,
};

// Returns true if valid position reading was found, false otherwise
static bool pos_from_dists(sonic_data_t dists[], vec_3d_t *pos)
{
    // Set clear init value for debugging in case of function failure
    *pos = (vec_3d_t) { .x = NO_INTERSECTION, .y = NO_INTERSECTION, .z = NO_INTERSECTION };

    // First, ascertain x and y coordinates of object in plane of the board. Do this by conceptualizing
    // each scalar ultrasonic reading as a "sphere" of possible object locations around the sensor,
    // with radius equal to the distance read. Then find the intersections of these spheres,
    // which will always be circles with plane perpendicular to the x or y axis connecting the sphere
    // centers. The location of this circle along the connecting axis is the x or y coord we are
    // trying to find.

    circle_t left = xy_sphere_intersect(dists[SENSOR_TOP_LEFT].distance, dists[SENSOR_BOTTOM_LEFT].distance, RECT_HEIGHT);
    circle_t right = xy_sphere_intersect(dists[SENSOR_TOP_RIGHT].distance, dists[SENSOR_BOTTOM_RIGHT].distance, RECT_HEIGHT);
    circle_t top = xy_sphere_intersect(dists[SENSOR_TOP_RIGHT].distance, dists[SENSOR_TOP_LEFT].distance, RECT_WIDTH);
    circle_t bottom = xy_sphere_intersect(dists[SENSOR_BOTTOM_RIGHT].distance, dists[SENSOR_BOTTOM_LEFT].distance, RECT_WIDTH);

    // If both readings are valid, average for noise reduction
    if (left.radius != NO_INTERSECTION && right.radius != NO_INTERSECTION) pos->y = (left.displacement + right.displacement) / 2;
    else if (left.radius != NO_INTERSECTION) pos->y = left.displacement;
    else if (right.radius != NO_INTERSECTION) pos->y = right.displacement;
    // Both pairs of spheres don't intersect; can't ascertain y direction. Function fails.
    else return false;

    if (top.radius != NO_INTERSECTION && bottom.radius != NO_INTERSECTION) pos->x = (top.displacement + bottom.displacement) / 2;
    else if (top.radius != NO_INTERSECTION) pos->x = top.displacement;
    else if (bottom.radius != NO_INTERSECTION) pos->x = bottom.displacement;
    // Both pairs of spheres don't intersect; can't ascertain x direction. Function fails.
    else return false;

    // Now, find the z coordinate of the object - its height above the board. This is done by
    // looking at the intersection of the aforementioned spheres with the circles we just calculated
    // (which themselves are intersections of two spheres). We look at the intersection of a given
    // circle with either of the two spheres not involved in the circle's "creation" to find z.
    // This is because intersection of a sphere with a skew circle in this fashion (assuming
    // an intersection point exists) will give only 2 points, one of which is behind the board
    // and we know our sensors cannot detect. So we choose the one with positive z.

    // Traverse 4 out of 8 possible combinations of spheres and skew circles.
    // If one is found, continue until a second is found so we can average the two
    // resulting z values for greater accuracy.
    bool found_z = false;
    if (sphere_circle_hit(dists[SENSOR_TOP_LEFT].distance, right.radius, RECT_HEIGHT / 2, RECT_WIDTH)) {
        pos->z = z_circle_sphere_intersect(dists[SENSOR_TOP_LEFT].distance, right.radius, RECT_HEIGHT / 2, RECT_WIDTH);
        found_z = true;
    }
    if (sphere_circle_hit(dists[SENSOR_TOP_RIGHT].distance, bottom.radius, RECT_WIDTH / 2, RECT_HEIGHT)) {
        float height = z_circle_sphere_intersect(dists[SENSOR_TOP_RIGHT].distance, bottom.radius, RECT_WIDTH / 2, RECT_HEIGHT);
        if (found_z) { // Average both values and we're done
            pos->z = (pos->z + height) / 2;
            return true;
        } else {
            pos->z = height;
            found_z = true;
        }
    }
    if (sphere_circle_hit(dists[SENSOR_BOTTOM_RIGHT].distance, left.radius, RECT_HEIGHT / 2, RECT_WIDTH)) {
        float height = z_circle_sphere_intersect(dists[SENSOR_BOTTOM_RIGHT].distance, left.radius, RECT_HEIGHT / 2, RECT_WIDTH);
        if (found_z) {
            pos->z = (pos->z + height) / 2;
            return true;
        } else {
            pos->z = height;
            found_z = true;
        }
    }
    if (sphere_circle_hit(dists[SENSOR_BOTTOM_LEFT].distance, top.radius, RECT_WIDTH / 2, RECT_HEIGHT)) {
        float height = z_circle_sphere_intersect(dists[SENSOR_BOTTOM_LEFT].distance, top.radius, RECT_WIDTH / 2, RECT_HEIGHT);
        if (found_z) {
            pos->z = (pos->z + height) / 2;
            return true;
        } else {
            pos->z = height;
            found_z = true;
        }
    }

    // If we only found one total height value we're also done
    if (found_z) return true;

    // No z values were found from the method above, so we give our best guess for z.
    // We never fail the function on under-determined z because we can approximate z fairly well.

    // Find minimum of all sphere intersection heights: object is at least this close in z
    float z_guess = MAX_SENSE_DEPTH;
    if (left.radius != NO_INTERSECTION) z_guess = min(left.radius, z_guess);
    if (right.radius != NO_INTERSECTION) z_guess = min(right.radius, z_guess);
    if (top.radius != NO_INTERSECTION) z_guess = min(top.radius, z_guess);
    if (bottom.radius != NO_INTERSECTION) z_guess = min(bottom.radius, z_guess);
    pos->z = z_guess;

    return true;
}
// --------------- END 3D POSITION MODULE ---------------


// --------------- BEGIN HIT PREDICTION MODULE ---------------
static inline vec_3d_t vec_add(vec_3d_t v1, vec_3d_t v2)
{
    return (vec_3d_t){ .x = v1.x + v2.x, .y = v1.y + v2.y, .z = v1.z + v2.z };
}

static inline vec_3d_t vec_sub(vec_3d_t pos_v, vec_3d_t neg_v)
{
    return (vec_3d_t){ .x = pos_v.x - neg_v.x, .y = pos_v.y - neg_v.y, .z = pos_v.z - neg_v.z };
}

static inline vec_3d_t vec_div(vec_3d_t v, float scalar)
{
    return (vec_3d_t){ .x = v.x / scalar, .y = v.y / scalar, .z = v.z / scalar };
}

typedef struct {
    vec_3d_t pos;
    vec_3d_t vel;
    vec_3d_t accel;
} kinematic_t;

// IMPORTANT: Assumes `n_positions` is at least 3 (needed to calc velocity and accel),
// and assumes `positions` and `timestamps` arrays are of same length as `n_positions`.

// The `timestamps` array contains a timestamp for each position reading, taken from
// the middle sensor to fire (in a temporal sense) from the array.
static kinematic_t trajec_from_positions(vec_3d_t positions[], unsigned timestamps[], int n_positions)
{
    // Velocity data will have length of position data - 1
    // Accel data will have length of velocity data - 1
    // (data for nth derivative of position will have length "posdatalen" - n;
    // if n or fewer total pos data points then nth derivative is undefined)
    int n_vels = n_positions - 1;
    int n_accels = n_positions - 2;
    vec_3d_t *vels = malloc(n_vels * sizeof(vec_3d_t));
    vec_3d_t *accels = malloc(n_accels * sizeof(vec_3d_t));

    // Get timestamp of reading for init pos and final pos to determine dt
    // and thus velocity: v = dr/dt ~= (r_final - r_init) / dt.
    for (int i = 0; i < n_vels; i++) {
        unsigned dt_micros = timestamps[i + 1] - timestamps[i];
        vels[i] = vec_div(vec_sub(positions[i + 1], positions[i]), dt_micros);
    }

    // Use "outside" timestamps of first and last of 3 points to calc accel
    for (int i = 0; i < n_accels; i++) {
        unsigned dt_micros = timestamps[i + 2] - timestamps[i];
        accels[i] = vec_div(vec_sub(vels[i + 1], vels[i]), dt_micros);
    }

    // Average velocity and accel values for final result
    vec_3d_t vels_avg = { .x = 0, .y = 0, .z = 0 };
    for (int i = 0; i < n_vels; i++) {
        vels_avg = vec_add(vels_avg, vels[i]);
    }
    vels_avg = vec_div(vels_avg, n_vels);

    vec_3d_t accels_avg = { .x = 0, .y = 0, .z = 0 };
    for (int i = 0; i < n_accels; i++) {
        accels_avg = vec_add(accels_avg, accels[i]);
    }
    accels_avg = vec_div(accels_avg, n_accels);

    free(vels);
    free(accels);

    // Use middle position reading for final result
    return (kinematic_t) { .pos = positions[n_positions / 2], .vel = vels_avg, .accel = accels_avg };
}

// Result is returned by parameter passing (board_pos_t *); directly returns true if ANY
// intersection with the xy-plane will happen in the future (even if it's outside
// the bounds of the board), false otherwise
static bool intersec_from_trajec(kinematic_t obj_trajec, board_pos_t *intersec)
{
    // Get time until object hits board (t when z(t) == 0)
    float v_z = obj_trajec.vel.z;
    float a_z = obj_trajec.accel.z;
    float z = obj_trajec.pos.z;
    board_pos_t board;
    // Check discriminate of quadratic; if < 0 then obj will never hit board
    if ((v_z * v_z - 2 * a_z *z) > 0) {
        // Use calculated time to predict (x, y) pos of obj when it hits the board.
        // Takes advantage of fact that in any coordinate system, orthogonal components
        // are independent (x(t), y(t) can be computed independent of z(t)'s value)
        // (We assume constant acceleration when predicting)
        float t1 = (-v_z + sqrt(v_z * v_z - 2 * a_z *z)) / a_z;
        float t2 = (-v_z - sqrt(v_z * v_z - 2 * a_z *z)) / a_z;
        // Constrain times to positive (i.e. in the future); we don't care if object would have
        // hit the board in the past w/ its current trajectory
        t1 = max(0, t1);
        t2 = max(0, t2);
        float time_earlier = min(t1, t2);
        // Kinematics equations
        board.x = obj_trajec.pos.x + obj_trajec.vel.x * time_earlier + 0.5 * obj_trajec.accel.x * square(time_earlier);
        board.y = obj_trajec.pos.y + obj_trajec.vel.y * time_earlier + 0.5 * obj_trajec.accel.y * square(time_earlier);
    } else {
        board.x = board.y = 0;
        return false;
    }
    *intersec = board;
    return true;
}
// --------------- END HIT PREDICTION MODULE ---------------


// --------------- BEGIN PUBLIC API ---------------
// Returns false if no intersection with board and therefore nothing to write to `prediction`. Also
// returns false if it couldn't get enough reliable data to make a prediction. Returns true
// if a valid prediction was made and the hoop should be moved
#define N_BURST_SAMPLES 7
bool object_vector_predict(board_pos_t *prediction)
{
    // TODO: In addition to pruning data for only valid position readings as below,
    // do a linear regression and discard outliers. If r value is below a
    // certain minimum threshold, or there are so many outliers discarded that
    // there are less than 3 remaining position vectors, the function fails.

    // TODO: If burst method doesn't work, try retrying position reading until
    // enough valid positions are found? They don't necessarily have to be
    // super close together...

    sonic_data_t *array_readings[N_BURST_SAMPLES];
    // 3 passed as 3rd arg because >=3 sensors needed to triangulate position on any given read
    if (!sonic_read_sync_multiple(array_readings, N_BURST_SAMPLES, 3)) return false;
    // Keep only valid position readings from total number of readings
    vec_3d_t positions[N_BURST_SAMPLES];
    int n_positions = 0;
    for (int i = 0; i < N_BURST_SAMPLES; i++) {
        if (pos_from_dists(array_readings[i], &positions[n_positions])) n_positions++;
    }

    // Assemble timestamp array
    unsigned timestamps[N_BURST_SAMPLES];
    for (int i = 0; i < n_positions; i++) {
        // `N_SENSORS / 2` to use middle sensor for position timestamp
        timestamps[i] = array_readings[i][N_SENSORS / 2].timestamp;
    }
    for (int i = 0; i < N_BURST_SAMPLES; i++) {
        free(array_readings[i]);
    }
    kinematic_t trajec = trajec_from_positions(positions, timestamps, n_positions);
    if (!intersec_from_trajec(trajec, prediction)) return false;

    // Convert from bottom-left-corner origin coordinate system to center-of-rect origin coord system
    // (needed by motors to drive hoop)
    prediction->x -= RECT_WIDTH / 2;
    prediction->y -= RECT_HEIGHT / 2;
    return true;
}
// --------------- END PUBLIC API ---------------
