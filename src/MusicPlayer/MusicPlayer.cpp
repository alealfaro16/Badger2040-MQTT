#include "MusicPlayer.h"
#include "hardware/pwm.h"
#include <math.h>


//Adapt this function to methods in your class
uint32_t pwm_set_freq_duty(uint slice_num,
       uint chan,uint32_t f, int d)
{
	uint32_t clock = 125000000;
	uint32_t divider16 = clock / f / 4096 + 
		(clock % (f * 4096) != 0);
	if (divider16 / 16 == 0)
		divider16 = 16;
	uint32_t wrap = clock * 16 / divider16 / f - 1;
	pwm_set_clkdiv_int_frac(slice_num, divider16/16,
						 divider16 & 0xF);
	pwm_set_wrap(slice_num, wrap);
	pwm_set_chan_level(slice_num, chan, wrap * d / 100);
	return wrap;
}

MusicPlayer::MusicPlayer(int pin) :pwmSlice(pwm_gpio_to_slice_num(pin)), pwmChan(pwm_gpio_to_channel(pin)){

	//Init PWM for buzzer
	gpio_set_function(pin, GPIO_FUNC_PWM);
	uint32_t wrap =  pwm_set_freq_duty(pwmSlice, pwmChan, NOTE_B2, 100);
	pwm_set_enabled(pwmSlice, true);//Define the duration of a whole note in ms and calculate the number of tones in the song

	wholeNote = (60000*4)/tempo;
	// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
	// there are two values per note (pitch and duration), so for each note there are four bytes
	toneNum = sizeof(nokiaMelody)/sizeof(nokiaMelody[0])/2;
	//Create timer to play song
	auto songTimerCallback = [](TimerHandle_t timer) {
		MusicPlayer* player = static_cast<MusicPlayer*>(pvTimerGetTimerID(timer));
		assert(player);
		player->playSong();
	};
	songTimer = xTimerCreate("Song timer", pdMS_TO_TICKS(300), pdTRUE, static_cast<void*>(this), songTimerCallback);
	if( xTimerStart( songTimer, 0 ) != pdPASS )
	{
		/* The timer could not be set into the Active
				 state. */
		LogError(("Song timer couldn't be started"));
	}
}

void MusicPlayer::playTone(int freq) {
	pwm_set_freq_duty(pwmSlice, pwmChan, freq, 75);
}

void MusicPlayer::silenceSpeaker(void) {
	pwm_set_chan_level(pwmSlice, pwmChan, 0);
}

void MusicPlayer::playSong(void) {

	//std::string tone = initSong[toneIndex];
	if (toneIndex == toneNum) {
		xTimerStop(songTimer, 0);
		toneIndex = 0;
		silenceSpeaker();
		return;
	}
	int divider = nokiaMelody[2*toneIndex + 1];
	int noteDuration = 0;
	if (divider > 0) {
		// regular note, just proceed
		noteDuration = (wholeNote) / divider;
	} else if (divider < 0) {
		// dotted notes are represented with negative durations!!
		noteDuration = (wholeNote) / abs(divider);
		noteDuration *= 1.5; // increases the duration in half for dotted notes
	}

	if (nokiaMelody[2*toneIndex] == 0) silenceSpeaker();
	else {
		//assert(toneMap.find(tone) != toneMap.end());
		playTone(nokiaMelody[2*toneIndex]);
	}
	if( xTimerChangePeriod( songTimer, noteDuration/ portTICK_PERIOD_MS, 100 )
                                                            != pdPASS ) {
		LogError(("Couldn't change the period of the timer!"));
	}

	toneIndex++;
}
