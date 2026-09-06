#ifndef __BC_PIEZOELECTRIC_MOBOR_H__
#define __BC_PIEZOELECTRIC_MOBOR_H__



#include <stdint.h>
#include <stdbool.h>
#include "bos1921.h"

struct __attribute__((__packed__)) bc_slice_parameters
{
	struct bos1921_slice_parameters slice_parameters;
	uint8_t slice_count;
};


void bc_piezoelectric_motor_chip_id(void);

uint8_t bc_piezoelectric_motor_chip_id_get(void);

bool bc_piezoelectric_motor_chip_id_check(void);


void bc_piezoelectric_motor_test_play(void);

void bc_piezoelectric_motor_test(void);

void bc_piezoelectric_motor_play(struct bc_slice_parameters *slice_parameters);


void bc_piezoelectric_motor_check(void);

void bc_piezoelectric_motor_play_tdk(void);
void bc_piezoelectric_motor_init(void);

#endif

