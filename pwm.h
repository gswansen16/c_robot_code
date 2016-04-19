#ifndef PWM_H
#define PWM_H

void pwm_cleanup(void);
int start_pwm(char *channel, float duty, float freq, int polarity);
int stop_pwm(char *channel);
int set_duty_cycle_pwm(char *channel, float duty);
int set_frequency_pwm(char *channel, float freq);

#endif