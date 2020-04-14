# CS 107e Final Project: Pi-a-Shot
### Adam Shugar and Ryan Johnston

## Context (for non-CS 107e folks)
CS 107e is a hardware-based alternative to CS 107, Stanford's first of two systems courses in the core undergrad CS curriculum. The assignments guide students through constructing a stripped-down C library to interface with Raspberry Pi hardware, culminating in an extensible graphical shell program that can echo text, get and set memory values, and disassemble binary ARM instructions in RPi memory. In lieu of a final exam, 107e students are given 2 weeks and a $40 budget to construct a project using the skills they've learned. More info is available [here](http://cs107e.github.io/about/).

Unfortunately, the situation due to COVID-19 necessaitated that the quarter be cut short by a week, and we had only 6 days to complete our project. The teaching team waived the final project requirement but we decided to keep working just for fun. Regardless, we thought it would be fun to share our results. We hope you enjoy!

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
1. [Demonstration of the motorized basketball hoop](https://drive.google.com/file/d/1hXNolnpdfb_8Lr7hBOtEoQXyCrvndfV1/view?usp=sharing)
2. [3D position sensing of a nearby object](https://drive.google.com/file/d/11HsTOugfwniYLjHLAmtBZrumcjuXgfk6/view?usp=sharing)
3. [Hardware used, build details, and wiring of project](https://drive.google.com/file/d/1NWbf1CsB6s67s9d4jwMaRE0bCrDz4SPi/view?usp=sharing)

#### Technical details
The code in this repository is also worth a brief description. All code from the assignments was imported and properly builds into a complete system for the Pi. The system includes all optional extra components: (i.e., custom implementations of a program profiler, shell history & Emacs commands, library functions for graphical lines & triangles, and a mini-Valgrind built into the allocator). The system code is compiled via local Makefile into a static library that is used by the project code. The instructor-provided static lib (`libpi.a`) is only used for the `gpioextra` module (easy GPIO state change-based interrupts). *Please note that in the public-facing version of this repo, student assignment code has been removed and commit history has been hidden to keep course content secure. Therefore, the project code will not successfully build as-is. If you are interested in viewing the source (and you are not a current or future student in 107e), please contact me at ashugar@stanford.edu.*

The actual project source is divided into 5 modules: the motor driver module (`motor.c`), the hoop moving module - uses the motor driver module (`hoop.c`), the ultrasonic sensor driver module (`sonic.c`; helper files `sonic_rb.c` and `countdown.c`), the object triangulation module - uses the ultrasonic sensor driver module (`object_vector.c`), and the coordinating module (`main.c`). The program obviously boots into `main.c` and infinite loops as quickly as possible on the following:

1. `get the nearby object's position, velocity, and acceleration vectors with respect to the center of the board in 3-dimensional space (stall until a valid reading of these values is obtained)` (`object_vector.c`)
2. `use this data to predict where the object will land in the plane of the hoop` (`object_vector.c`)
3. `move the hoop to that location` (`hoop.c`)

The position-sensing module in particular was a very interesting problem. We are starting with 4 scalar distance readings of the closest object to each ultrasonic sensor at each corner of the board, and we have to turn this into a 3D position vector from the board origin to the object. We solved it by conceptualizing each scalar reading as a sphere around its ultrasonic sensor, since the object could be anywhere X mm away from the sensor if the sensor returned the value X (i.e. a sphere with radius X mm). The object will then be roughly at the point where these 4 spheres intersect. The problem then becomes finding the 3D coordinate of this intersection point, also considering that any one of the sensors could time out (i.e. give no reading at all), and the other 3 could have significant noise in their readings. As seen in the video, after much toil, we solved it.

There are two methods of deployment for the project that we considered:
1. Flash the `pishot` source to an SD card and make it the program that runs on startup. This way, as soon as the system gets power it is working and doesn't need someone with access to the source repo, bootloader, RPi knowledge, etc. etc. This is the preferred method.
p2. Include the project as a shell command. The user could boot up the regular Pi shell and type a command like `pishot [seek|avoid|arrowkeys]` for different modes. (`seek` would go towards the ball, `avoid` would avoid the ball, and `arrowkeys` would let a second user control the hoop location with arrowkeys, turning off ball position sensing. `ctrl-C` would terminate the `pishot` program.)

#### For the especially interested
We got each individual component working, but we didn't have time to put on the finishing touches and make the entire system work together. There were two main hurdles to doing so, which actually easily addressed with a bit more time:
1. **Problem:** We had to extend the diameter of the stepper motors to increase the maximum speed of the hoop. To do so, we superglued a circular duct tape "wrap" around each motor shaft. However, the fishing line we used would frequently slip under the wrap onto the bare shaft or over the top of the "wrap" and off of the motor shaft altogether. We combatted this by supergluing cardboard "tracks" on the top and bottom of the wrap to keep the fishing line in place, but this was flimsy and error-prone. **Solution:** 3D-print a part that slips over the motor shaft, extending its diameter, with protruding tracks on top and bottom to keep the fishing line in place. Design this part using Solidworks. (This actually isn't too hard to do.) The on-campus Product Realization Lab was closed due to COVID; otherwise we would've done this.
2. **Problem:** We need to move the hoop between any two points 1 and 2 on the board. We thought this just meant individually moving each motor at constant speed from `amount of slack at point 1` to `amount of slack at point 2`. However, there is an additional layer of complexity: at every point along the path, every motor needs to be in slight tension. Otherwise, if too much fishing line gets loose, it won't re-spool properly. **Solution:** Update the motor driver algorithm to drive the motors at non-constant speeds for a given path. We didn't have time to rewrite the algorithm to handle this, but it's also a relatively simple fix.

As soon as campus reopens, we plan to finish the project as detailed. Not quite sure where this 8-foot-tall thing will live during the school year though. :)
