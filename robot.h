#ifndef ROBOT_H
#define ROBOT_H

#import "gpio.h"
#import "mongoose.h"
#import <stdio.h>
#import <stdlib.h>
#import <unistd.h>

#define SERVER_PORT "8000"
#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct file_node {
	char* file_name;
	char* file_data;
	long file_size;
	struct file_node* next;
};

struct file_node *html_pages = NULL;

char* private_files[] = {"private/robot_control.html"};

char robot_commands[512] = {0};

#endif