#pragma once

#include "FreeRTOS.h"
#include "timers.h"
//#define LIBRARY_LOG_NAME "MusicPlayer"
//#define LIBRARY_LOG_LEVEL LOG_DEBUG
#include "logging_config.h"
#include "logging_stack.h"

#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "MusicDefs.h"


class MusicPlayer {

	public:
	MusicPlayer(int pin);
	~MusicPlayer() {};
	void playSong(void);

	private:
	int wholeNote;
	int tempo = 180;
	int toneNum;
	TimerHandle_t songTimer;
	int toneIndex = 0;
	int noteDuration;
	uint8_t pwmSlice;
	uint8_t pwmChan;
	uint32_t pwmSet_freq_duty(uint slice_num,
		uint chan,uint32_t f, int d);
	void playTone(int freq);
	void silenceSpeaker(void);
};
