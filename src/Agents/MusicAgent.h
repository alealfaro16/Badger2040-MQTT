/*
 * BlinkAgent.h
 *
 * Active agent to run as task and blink and LED on the given GPIO pad
 *
 *  Created on: 15 Aug 2022
 *      Author: jondurrant
 */

#ifndef MUSICAGENT_H_
#define MUSICAGENT_H_

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "Agent.h"
#define SINE_WAVE_TABLE_LEN 2048
#define SAMPLES_PER_BUFFER 256

class MusicAgent: public Agent {
public:
	/***
	 * Constructor
	 * @param gp - GPIO Pad number for LED
	 */
	MusicAgent(uint8_t pin=0);

	/***
	 * Destructor
	 */
	virtual ~MusicAgent();


protected:

	/***
	 * Run loop for the agent.
	 */
	virtual void run();

	struct audio_buffer_pool *initAudio();


	/***
	 * Get the static depth required in words
	 * @return - words
	 */
	virtual configSTACK_DEPTH_TYPE getMaxStackSize();

	//GPIO PAD for LED
	uint8_t pwmPin = 0;
	int16_t sine_wave_table[SINE_WAVE_TABLE_LEN];

};


#endif /* MUSICAGENT_H_ */
