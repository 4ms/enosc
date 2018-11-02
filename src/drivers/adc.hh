
#pragma once

#include "hal.hh"

// ADC1
enum BuiltinAdc1Channels{
	WARP_POT_ADC,		//  0
	DETUNE_POT_ADC,		//  1
	MOD_POT_ADC,		//  2
	ROOT_POT_ADC, 		//  3
	GRID_POT_ADC,		//  4
	PITCH_POT_ADC,		//  5
	SPREAD_POT_ADC,		//  6
	TILT_POT_ADC,		//  7
	TWIST_POT_ADC,		//  8

	NUM_BUILTIN_ADC1
};

// ADC3
enum BuiltinAdc3Channels{
	SPREAD_CV_1_ADC,	//  0
	WARP_CV_ADC,		//  1
	SPREAD_CV_2_ADC,	//  2
	TWIST_CV_ADC, 		//  3
	TILT_CV_ADC,		//  4
	GRID_CV_ADC,		//  5
	MOD_CV_ADC,			//  6
	
	NUM_BUILTIN_ADC3
};

#define NUM_BUILTIN_ADCS	(NUM_BUILTIN_ADC1 + NUM_BUILTIN_ADC2 + NUM_BUILTIN_ADC3)
#define NUM_ADCS 			(NUM_HIRES_ADCS + NUM_BUILTIN_ADCS)

void adc_init_all(void);
