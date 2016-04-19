#include "gpio.h"
#include <stdio.h>

int main(int argc, char** argv){
	char* pin = "P9_14";

	printf("%i\n", setup_channel_gpio(pin, OUTPUT, 0));
	printf("%i\n", output_gpio(pin, 0));

	return 0;
}