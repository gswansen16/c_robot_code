#ifndef GPIO_H
#define GPIO_H

#define INPUT  0
#define OUTPUT 1
#define ALT0   4

#define HIGH 1
#define LOW  0

#define MAX_FILENAME 50

#define USR_LED_GPIO_MIN 53
#define USR_LED_GPIO_MAX 56

#define PUD_OFF  0
#define PUD_DOWN 1
#define PUD_UP   2

int setup_channel_gpio(char* channel, int direction, int initial);
int output_gpio(char* channel, int value);
int input_gpio(char* channel, unsigned int* value);
void cleanup_gpio(void);

#endif