#ifndef ROBOT_H
#define ROBOT_H

#include "pwm.h"
#include "gpio.h"
#include "mongoose.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define SERVER_PORT "8000"
#define SERVER_ADDRESS "0.0.0.0"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define LEFT_MOTOR "P9_16"
#define RIGHT_MOTOR "P9_22"
#define DISTANCE_SENSOR "P9_11"

//The following values define the operating frequency and duty cycle for the motor controller. 
//For example if the motor controller was set with a frequency of 333 HZ and a duty cycle of 50% it should be at rest.
//For the motor to be going completely forward it should be at a frequency of 333 HZ and a duty cycle of 50% + 16.66%.
#define PWM_MOTOR_FREQUENCY 333
#define PWM_MOTOR_DUTY_CYCLE_CENTER 50.00F
#define PWM_MOTOR_DUTY_CYCLE_RANGE 16.66F

struct file_node {
	char* file_name;
	char* file_data;
	long file_size;
	struct file_node* next;
};

struct file_node *html_pages = NULL;

typedef char* mime_pairs[2];

mime_pairs pairs[] = {
	{"jar", "application/java-archive"},
	{"html", "text/html"},
	{"js", "application/javascript"},
	{"txt", "text/plain"}
};

char* private_files[] = {"private/robot_control.html"};

unsigned char robot_commands[512] = {0};
int thread_running = 1;

#endif