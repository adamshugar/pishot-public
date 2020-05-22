# Pi-a-Shot
### Adam Shugar and Ryan Johnston

## Project Description
#### Summary
The basic idea of the project is to create an "assistive basketball hoop" that moves toward an incoming ball so that even an inaccurately thrown ball ends up in the hoop. We built a motion tracking system that can determine the 3D location of a ball multiple times per second based on input data from an array of ultrasonic time-of-flight (TOF) HC-SR04 distance sensors. We use this data to drive NEMA-17 stepper motors that change the x, y coordinates of the hoop so that it "meets" the ball in the proper location.

#### Hardware Used
* 4 HC-SR04 ultrasonic sensors
* 4 NEMA-17 stepper motors
* 4 A4988 stepper motor drivers
* Toy basketball hoop and toy basketballs
* 11.1V battery and power cable
* Nylon wire to connect stepper motors to hoop
* Sliding metal tracks to change position of hoop
* Wood frame to house entire system (custom-built)

## Project Results - March 14th 2020

Here are some brief videos documenting our final results:
1. [3D position sensing of a nearby object](https://drive.google.com/file/d/11HsTOugfwniYLjHLAmtBZrumcjuXgfk6/view?usp=sharing)
2. [Demonstration of the motorized basketball hoop](https://drive.google.com/file/d/1hXNolnpdfb_8Lr7hBOtEoQXyCrvndfV1/view?usp=sharing)
3. [Hardware used, build details, and wiring of project](https://drive.google.com/file/d/1NWbf1CsB6s67s9d4jwMaRE0bCrDz4SPi/view?usp=sharing)

#### Technical details
The project source is divided into 5 modules: the motor driver module (`motor.c`), the hoop moving module - uses the motor driver module (`hoop.c`), the ultrasonic sensor driver module (`sonic.c`; helper files `sonic_rb.c` and `countdown.c`), the object triangulation module - uses the ultrasonic sensor driver module (`object_vector.c`), and the coordinating module (`main.c`). The program boots into `main.c` and infinite loops as quickly as possible on the following:

1. `get the nearby object's position, velocity, and acceleration vectors with respect to the center of the board in 3-dimensional space (stall until a valid reading of these values is obtained)` (`object_vector.c`)
2. `use this data to predict where the object will land in the plane of the hoop` (`object_vector.c`)
3. `move the hoop to that location` (`hoop.c`)

The position-sensing module in particular was a very interesting problem. We are starting with 4 scalar distance readings of the closest object to each ultrasonic sensor at each corner of the board, and we have to turn this into a 3D position vector from the board origin to the object. We solved it by conceptualizing each scalar reading as a sphere around its ultrasonic sensor, since the object could be anywhere X mm away from the sensor if the sensor returned the value X (i.e. a sphere with radius X mm). The object will then be roughly at the point where these 4 spheres intersect. The problem then becomes finding the 3D coordinate of this intersection point, also considering that any one of the sensors could time out (i.e. give no reading at all), and the other 3 could have significant noise in their readings. As can be seen in the video, it works pretty well.

Project deployment: we flash the `pishot` source to an SD card so that it runs automatically on RPi startup.
