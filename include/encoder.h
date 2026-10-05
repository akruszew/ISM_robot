#ifndef ENCODER_H
#define ENCODER_H
#include <stdint.h>

bool encoder_init(void);
int32_t encoder_get_ticks(void);
int32_t encoder_get_and_reset_delta(void);
void encoder_reset(void);



#endif