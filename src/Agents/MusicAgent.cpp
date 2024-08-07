/*
 * MusicAgent.cpp
 *
 *  Created on: 15 Aug 2022
 *      Author: jondurrant
 */

#include "MusicAgent.h"

#include "stdio.h"
#include "pico/audio_pwm.h"
#include <cmath>


//Music definitions

/***
 * Constructor
 * @param gp - GPIO Pad number for LED
 */
MusicAgent::MusicAgent(uint8_t pin) {
	pwmPin = pin;

}

/***
 * Destructor
 */
MusicAgent::~MusicAgent() {
	stop();
}

struct audio_buffer_pool *MusicAgent::initAudio() {
	static audio_format_t audio_format = {
		.sample_freq = 24000,
		.format = AUDIO_BUFFER_FORMAT_PCM_S16,
		.channel_count = 1,
	};

	static struct audio_buffer_format producer_format = {
		.format = &audio_format,
		.sample_stride = 2
	};

	struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,SAMPLES_PER_BUFFER);

	bool __unused ok;
	const struct audio_format *output_format;
	audio_pwm_channel_config_t mono_channel_config =
		{
			.core = {
				.base_pin = pwmPin,
				.dma_channel = 0,
				.pio_sm = 0
			},
			.pattern = 3,
		};
	output_format = audio_pwm_setup(&audio_format, -1, &mono_channel_config);
	if (!output_format) {
		panic("PicoAudio: Unable to open audio device.\n");
	}
	ok = audio_pwm_default_connect(producer_pool, false);
	assert(ok);
	audio_pwm_set_enabled(true);
	return producer_pool;
}


 /***
  * Main Run Task for agent
  */
 void MusicAgent::run(){

	printf("Music Agent Started\n");
    for (int i = 0; i < SINE_WAVE_TABLE_LEN; i++) {
        sine_wave_table[i] = 32767 * cosf(i * 2 * (float) (M_PI / SINE_WAVE_TABLE_LEN));
    }

    struct audio_buffer_pool *ap = initAudio();
    uint32_t step = 0x200000;
    uint32_t pos = 0;
    uint32_t pos_max = 0x10000 * SINE_WAVE_TABLE_LEN;
    uint vol = 128;
    while (true) {
//#if USE_AUDIO_PWM
//        enum audio_correction_mode m = audio_pwm_get_correction_mode();
//#endif
//        int c = getchar_timeout_us(0);
//        if (c >= 0) {
//            if (c == '-' && vol) vol -= 4;
//            if ((c == '=' || c == '+') && vol < 255) vol += 4;
//            if (c == '[' && step > 0x10000) step -= 0x10000;
//            if (c == ']' && step < (SINE_WAVE_TABLE_LEN / 16) * 0x20000) step += 0x10000;
//            if (c == 'q') break;
//#if USE_AUDIO_PWM
//            if (c == 'c') {
//                bool done = false;
//                while (!done) {
//                    if (m == none) m = fixed_dither;
//                    else if (m == fixed_dither) m = dither;
//                    else if (m == dither) m = noise_shaped_dither;
//                    else if (m == noise_shaped_dither) m = none;
//                    done = audio_pwm_set_correction_mode(m);
//                }
//            }
//            printf("vol = %d, step = %d mode = %d      \r", vol, step >>16, m);
//#else
//            printf("vol = %d, step = %d      \r", vol, step >> 16);
//#endif
//        }
//        struct audio_buffer *buffer = take_audio_buffer(ap, true);
//        int16_t *samples = (int16_t *) buffer->buffer->bytes;
//        for (uint i = 0; i < buffer->max_sample_count; i++) {
//            samples[i] = (vol * sine_wave_table[pos >> 16u]) >> 8u;
//            pos += step;
//            if (pos >= pos_max) pos -= pos_max;
//        }
//        buffer->sample_count = buffer->max_sample_count;
//        give_audio_buffer(ap, buffer);
    }

 }

/***
 * Get the static depth required in words
 * @return - words
 */
configSTACK_DEPTH_TYPE MusicAgent::getMaxStackSize(){
	return 1024*2;

}
