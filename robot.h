#ifndef ROBOT_H
#define ROBOT_H

#include "pwm.h"
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

char* private_files[] = {"private/robot_control.html"};

unsigned char robot_commands[512] = {0};
int thread_running = 1;

#endif