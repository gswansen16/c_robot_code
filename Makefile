CC = gcc
CFLAGS = -Wall 
FILES = mongoose.c robot.c adc.c common.c gpio.c pwm.c
robot: $(OBJECTS)
	$(CC) -o robot $(FILES) $(CFLAGS)