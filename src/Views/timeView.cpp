#include "TimeView.h"
#include "hardware/rtc.h"
#include "BadgerAgent.h"
#include <math.h>
#include <string.h>

void TimeView::drawClock(uint8_t x, uint8_t y, uint8_t handLen, uint8_t hour, uint8_t min){

	float a;
	int x2, y2;

	float handShort = (float)handLen * 0.75;

	//Dial
	for (uint8_t i=0; i < 12; i++){
		a = (float)i/12.0 * 6.28319;
		a = a + (1.0/12.0)*(float)min/60.0 * 6.28319;
		x2 = sin(a)*  handLen;
		y2 = cos(a)* handLen;
		badger.pixel(x+x2, y-y2);
	}

	//Hour hand
	a = (float)hour/12.0 * 6.28319;
	x2 = sin(a)*  handShort;
	y2 = cos(a)* handShort;
	badger.line(x, y, x+x2, y-y2);

	//Minute hand
	a = (float)min/60.0 * 6.28319;
	x2 = sin(a)* (float) handLen;
	y2 = cos(a)* (float) handLen;
	badger.line(x, y, x+x2, y-y2);
}
void TimeView::displayView(void) {
	
	datetime_t d;
	char dateStr[20];
	char timeStr[20];

	if (rtc_get_datetime(&d)) {
    		LogDebug(("RTC: %d-%d-%d %d:%d:%d\n",
    				d.year,
					d.month,
					d.day,
					d.hour,
					d.min,
					d.sec));

		snprintf(dateStr,20, "%d-%d-%d",d.day,d.month,d.year);
		if (d.min < 10) {
			snprintf(timeStr, 20,"%d : 0%d",d.hour, d.min);
		}
		else {
			snprintf(timeStr, 20,"%d : %d",d.hour, d.min);
		}
		badger.pen(15);
		badger.clear();
		badger.pen(0);
		drawClock(DISPLAY_WIDTH/4, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/2, d.hour, d.min);
		badger.text(dateStr, DISPLAY_WIDTH/2 - 4*TEXT_PADDING, DISPLAY_HEIGHT/4, 0.75f);
		badger.text(timeStr, DISPLAY_WIDTH/2 + 2*TEXT_PADDING, DISPLAY_HEIGHT/2, 0.75f);
		badger.update();
    }
	else {
		LogError(("RTC is not initialized"));

		drawClock(DISPLAY_WIDTH/4, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/2, 6, 0);
		badger.update();
	}

}
