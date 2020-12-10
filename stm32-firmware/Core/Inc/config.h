/*
 * config.h
 *
 *  Created on: Oct 28, 2020
 *      Author: loczek
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#define LED_ON_SOUND_LEVEL 1700
#define BUFFER_SIZE 1024
#define ADC_SAMPLING_TIME ADC_SAMPLETIME_1CYCLE_5

// Probing frequency in Hz
#define PROBE_FREQUENCY 20000

// Set first value in buffer, for debugging
#define OVERWRITE_FIRST_VALUE
#define FIRST_BUFFER_OVR_VALUE 0xAABB
#define SECOND_BUFFER_OVR_VALUE 0xDEAD

#define TIMER2_PERIOD ((1000000/PROBE_FREQUENCY) - 1)
#define TOTAL_BUFFER_SIZE (BUFFER_SIZE * 2)

#endif /* INC_CONFIG_H_ */
