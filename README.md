## Dependencies
	There are the following dependencies for compiling the project, although almost all Unix OS's will have these dependencies built in.
		1. GNU Make
		2. GCC
		3. POSIX Compliant Headers

	Although those are the only required dependencies to compile the code there are more for it to run properly (as in moving motors) as the IO depends on a specific device tree overlay.
	It has been built specifically for the Angrstrom image on the BeagleBone Black.

## Installation
	To install download the source, unpack it and run the following commands:

	```bash
	cd DIRECTORY_OF_PROJECT
	make
	```
  
## Running/Usage
	To make sure the motors move properly ensure that they are given a 5V power supply, grounded properly with the left motor control being attached to pin P9_16 and the right motor control being attached to pin P9_22.

	To run the project after it is compiled with make simply run the binary robot.

	```bash
	./robot
	```