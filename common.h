#ifndef COMMON_H
#define COMMON_H

#define MODE_UNKNOWN -1
#define BOARD        10
#define BCM          11

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof(a[0]))

#define FILENAME_BUFFER_SIZE 128

char ctrl_dir[35];
char ocp_dir[25];

int get_gpio_number(const char *key, unsigned int *gpio);
int get_pwm_key(const char *input, char *key);
int get_adc_ain(const char *key, unsigned int *ain);
int get_uart_device_tree_name(const char *name, char *dt);
int build_path(const char *partial_path, const char *prefix, char *full_path, size_t full_path_len);
int get_spi_bus_path_number(unsigned int spi);
int load_device_tree(const char *name);
int unload_device_tree(const char *name);
int setup_error;
int module_setup;

#endif