#include "saw_osc.h"

uint32_t freq[2][2] = {{10000, -20000}, {20000, -20000}};
int32_t range_max[2] = {6710885, 3733335};
int32_t range_min[2] = {-6710885, -3733335};
int32_t saw[2] = {0};
int32_t saw_dir[2] = {0};

void set_saw_ranges(uint32_t new_range_min1, uint32_t new_range_max1, uint32_t new_range_min2, uint32_t new_range_max2) {
    range_min[0] = new_range_min1;
    range_max[0] = new_range_max1;
    range_min[1] = new_range_min2;
    range_max[1] = new_range_max2;
}

void set_saw_freqs(uint32_t new_freq_rise1, uint32_t new_freq_fall1, uint32_t new_freq_rise2, uint32_t new_freq_fall2) {
    freq[0][0] = new_freq_rise1;
    freq[0][1] = new_freq_fall1;
    freq[1][0] = new_freq_rise2;
    freq[1][1] = new_freq_fall2;
}

void saw_out(int32_t *dst, uint32_t frames)
{
    for (uint32_t i=0; i<frames; i++) {
        for (uint32_t j=0;j<2;j++)
        {
            saw[j] += freq[j][ saw_dir[j] ];

            if (saw[j] >= range_max[j] && saw_dir[j]==0) {
                saw_dir[j] = 1;
                *dst++ = range_max[j];
            }
            else if (saw[j] <= range_min[j] && saw_dir[j]==1) {
                saw_dir[j] = 0;
                *dst++ = range_min[j];
            }
            else
                *dst++ = saw[j];
        }
    }
};
