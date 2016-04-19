#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "gpio.h"
#include "common.h"

// file descriptors
struct fdx
{
    int fd;
    unsigned int gpio;
    struct fdx *next;
};

struct fdx *fd_list = NULL;

// gpio exports
struct gpio_exp
{
    unsigned int gpio;
    struct gpio_exp *next;
};
struct gpio_exp *exported_gpios = NULL;

int gpio_direction[120] = {-1};
int gpio_mode;

int gpio_export(unsigned int gpio)
{
    int fd, len;
    char str_gpio[10];
    struct gpio_exp *new_gpio, *g;

    if ((fd = open("/sys/class/gpio/export", O_WRONLY)) < 0)
    {
        return -1;
    }

    len = snprintf(str_gpio, sizeof(str_gpio), "%d", gpio);
    write(fd, str_gpio, len);
    close(fd);

    // add to list
    new_gpio = malloc(sizeof(struct gpio_exp));
    if (new_gpio == 0)
        return -1; // out of memory

    new_gpio->gpio = gpio;
    new_gpio->next = NULL;

    if (exported_gpios == NULL)
    {
        // create new list
        exported_gpios = new_gpio;
    } else {
        // add to end of existing list
        g = exported_gpios;
        while (g->next != NULL)
            g = g->next;
        g->next = new_gpio;
    }
    return 0;
}

void close_value_fd(unsigned int gpio)
{
    struct fdx *f = fd_list;
    struct fdx *temp;
    struct fdx *prev = NULL;

    while (f != NULL)
    {
        if (f->gpio == gpio)
        {
            close(f->fd);
            if (prev == NULL)
                fd_list = f->next;
            else
                prev->next = f->next;
            temp = f;
            f = f->next;
            free(temp);
        } else {
            prev = f;
            f = f->next;
        }
    }
}

int fd_lookup(unsigned int gpio)
{
    struct fdx *f = fd_list;
    while (f != NULL)
    {
        if (f->gpio == gpio)
            return f->fd;
        f = f->next;
    }
    return 0;
}

int add_fd_list(unsigned int gpio, int fd)
{
    struct fdx *new_fd;

    new_fd = malloc(sizeof(struct fdx));
    if (new_fd == 0)
        return -1;  // out of memory

    new_fd->fd = fd;
    new_fd->gpio = gpio;
    if (fd_list == NULL) {
        new_fd->next = NULL;
    } else {
        new_fd->next = fd_list;
    }
    fd_list = new_fd;
    return 0;
}

int open_value_file(unsigned int gpio)
{
    int fd;
    char filename[MAX_FILENAME];

    // create file descriptor of value file
    if((gpio >= USR_LED_GPIO_MIN) && (gpio <=  USR_LED_GPIO_MAX)) {
        snprintf(filename, sizeof(filename), "/sys/class/leds/beaglebone:green:usr%d/brightness", gpio -  USR_LED_GPIO_MIN);
    } else {
        snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio);
    }

    if((fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0)
        return -1;

    add_fd_list(gpio, fd);
    return fd;
}

int gpio_unexport(unsigned int gpio)
{
    int fd, len;
    char str_gpio[10];
    struct gpio_exp *g, *temp, *prev_g = NULL;

    close_value_fd(gpio);

    if ((fd = open("/sys/class/gpio/unexport", O_WRONLY)) < 0)
        return -1;

    len = snprintf(str_gpio, sizeof(str_gpio), "%d", gpio);
    write(fd, str_gpio, len);
    close(fd);

    // remove from list
    g = exported_gpios;
    while (g != NULL)
    {
        if (g->gpio == gpio)
        {
            if (prev_g == NULL)
                exported_gpios = g->next;
            else
                prev_g->next = g->next;
            temp = g;
            g = g->next;
            free(temp);
        } else {
            prev_g = g;
            g = g->next;
        }
    }
        return 0;
}

int gpio_set_direction(unsigned int gpio, unsigned int in_flag)
{
        int fd;
        char filename[40];
        char direction[10] = { 0 };

        snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/direction", gpio);
        if ((fd = open(filename, O_WRONLY)) < 0)
            return -1;

        if (in_flag) {
            strncpy(direction, "out", ARRAY_SIZE(direction) - 1);
        } else {
            strncpy(direction, "in", ARRAY_SIZE(direction) - 1);
        }

        write(fd, direction, strlen(direction));
        close(fd);
        return 0;
}

int gpio_get_direction(unsigned int gpio, unsigned int *value)
{
    int fd;
    char direction[4] = { 0 };
    char filename[40];

    snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/direction", gpio);
    if ((fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0)
        return -1;

    lseek(fd, 0, SEEK_SET);
    read(fd, &direction, sizeof(direction) - 1);

    if (strcmp(direction, "out") == 0) {
        *value = OUTPUT;
    } else {
        *value = INPUT;
    }
 
    return 0;
}

int gpio_set_value(unsigned int gpio, unsigned int value)
{
    int fd;
    char filename[MAX_FILENAME];
    char vstr[10];

    if ((gpio >= USR_LED_GPIO_MIN) && (gpio <=  USR_LED_GPIO_MAX)) {
        snprintf(filename, sizeof(filename), "/sys/class/leds/beaglebone:green:usr%d/brightness", gpio -  USR_LED_GPIO_MIN);
    } else {
        snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio);
    }

    if ((fd = open(filename, O_WRONLY)) < 0)
        return -1;

    if (value) {
        strncpy(vstr, "1", ARRAY_SIZE(vstr) - 1);
    } else {
        strncpy(vstr, "0", ARRAY_SIZE(vstr) - 1);
    }

    write(fd, vstr, strlen(vstr));
    close(fd);
    return 0;
}

int gpio_get_value(unsigned int gpio, unsigned int *value)
{
    int fd = fd_lookup(gpio);
    char ch;

    if (!fd)
    {
        if ((fd = open_value_file(gpio)) == -1)
            return -1;
    }    

    lseek(fd, 0, SEEK_SET);
    read(fd, &ch, sizeof(ch));

    if (ch != '0') {
        *value = 1;
    } else {
        *value = 0;
    }

    return 0;
}

void cleanup_gpio(void)
{
    // unexport everything
    while(exported_gpios != NULL)
        gpio_unexport(exported_gpios->gpio);
}

// Returns 1 on success. 0 on failure.
int setup_channel_gpio(char* channel, int direction, int initial){
    if(direction != INPUT && direction != OUTPUT)
        return 0;

    unsigned int gpio;
    if(get_gpio_number(channel, &gpio))
        return 0;

    if(gpio_export(gpio) < 0)
        return 0;

    if(gpio_set_direction(gpio, direction) < 0)
        return 0;

    int ret = -1;
    if(direction == OUTPUT){
        ret = gpio_set_value(gpio, initial);
    }else{
        ret = gpio_set_value(gpio, PUD_OFF);
    }

    if(ret < 0)
        return 0;

    gpio_direction[gpio] = direction;
    return 1;
}

// Returns 1 on succes. 0 on failure.
int output_gpio(char* channel, int value){
    unsigned int gpio;

    if(get_gpio_number(channel, &gpio))
        return 0;

    if(gpio_direction[gpio] != OUTPUT)
        return 0;

    if(gpio_set_value(gpio, value) < 0)
        return 0;

    return 1;
}

//Returns 1 on success. Returns 0 on failure.
int input_gpio(char* channel, unsigned int* value){
    unsigned int gpio;

    if(get_gpio_number(channel, &gpio))
        return 0;

    if(gpio_direction[gpio] != INPUT && gpio_direction[gpio] != OUTPUT)
        return 0;

    if(gpio_get_value(gpio, value) < 0)
        return 0;

    return 1;
}