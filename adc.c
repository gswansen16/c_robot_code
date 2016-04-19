#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "adc.h"
#include "common.h"

char adc_prefix_dir[40];

int adc_initialized = 0;

int initialize_adc(void)
{
    char test_path[40];
    FILE *fh;
    
    if (adc_initialized) {
        return 1;
    }

    if (load_device_tree("cape-bone-iio")) {
        build_path("/sys/devices", "ocp.", ocp_dir, sizeof(ocp_dir));
        build_path(ocp_dir, "helper.", adc_prefix_dir, sizeof(adc_prefix_dir));
        //Modified this line to prevent buffer overflows.
        strncat(adc_prefix_dir, "/AIN", 4);

        // Test that the directory has an AIN entry (found correct devicetree)
        snprintf(test_path, sizeof(test_path), "%s%d", adc_prefix_dir, 0);
        
        fh = fopen(test_path, "r");

        if (!fh) {
            return 0; 
        }
        fclose(fh);

        adc_initialized = 1;
        return 1;
    }

    return 0;
}

int read_value(unsigned int ain, float *value)
{
    FILE * fh;
    char ain_path[40];
    int err, try_count=0;
    int read_successful;
    snprintf(ain_path, sizeof(ain_path), "%s%d", adc_prefix_dir, ain);
    
    read_successful = 0;

    // Workaround to AIN bug where reading from more than one AIN would cause access failures
    while (!read_successful && try_count < 3)
    {
        fh = fopen(ain_path, "r");

        // Likely a bad path to the ocp device driver 
        if (!fh) {
            return -1;
        }

        fseek(fh, 0, SEEK_SET);
        err = fscanf(fh, "%f", value);

        if (err != EOF) read_successful = 1;
        fclose(fh);

        try_count++;
    }

    if (read_successful) return 1;

    // Fall through and fail
    return -1;
}

int setup_adc()
{
    return initialize_adc();
}

void cleanup_adc(void)
{
    unload_device_tree("cape-bone-iio");
}

//Returns 1 on success. 0 on failure.
int read_adc(char* channel, float *value){
    unsigned int ain;

    if(!adc_initialized)
        return 0;

    if (!get_adc_ain(channel, &ain))
        return 0;

    if(read_value(ain, value) < 0)
        return 0;

    return 1;
}
