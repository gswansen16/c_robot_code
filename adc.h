#ifndef ADC_H
#define ADC_H

int setup_adc(void);
int read_adc(char* channel, float *value);
void cleanup_adc(void);

#endif