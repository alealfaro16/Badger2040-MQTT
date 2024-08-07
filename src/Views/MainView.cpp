#include "ViewUtil.h"
#include "MainView.h"
#include "View.h"
#include "hardware/rtc.h"
#include <math.h>

void MainView::displayInitView(void) {
	badger.clearToWhite();
	badger.font("serif");
	badger.thickness(DEFAULT_THICKNESS);
	badger.text("Initiating Badger....", TEXT_PADDING, TEXT_SPACING + TEXT_PADDING, TITLE_TEXT_SIZE);
	badger.update();
}

void MainView::displayMainView(void) {

	badger.clearToWhite();
	int eventNum = eventView->getEventNum();
	int remindNum = reminderView->getReminderNum();
	badger.text("Welcome! You Have:", TEXT_PADDING + DISPLAY_WIDTH/8, TITLE_TEXT_SPACING + TEXT_PADDING, TITLE_TEXT_SIZE);
	std::string status_str = std::to_string(eventNum) + " Events"; 
	badger.text(status_str, TEXT_PADDING + DISPLAY_WIDTH/4, 2*TITLE_TEXT_SPACING + TEXT_PADDING, TITLE_TEXT_SIZE);
	status_str = std::to_string(remindNum) + " Reminders"; 
	badger.text(status_str, TEXT_PADDING + DISPLAY_WIDTH/4, 3*TITLE_TEXT_SPACING + TEXT_PADDING, TITLE_TEXT_SIZE);

	drawSideLabels();
	badger.update();

}

void MainView::displayView(void) {
	displayFunction func = displayFuncs[screenIdx];
	(this->*func)();
}

void MainView::drawClock(uint8_t x, uint8_t y, uint8_t handLen, uint8_t hour, uint8_t min){

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
void MainView::displayClockView(void) {
	
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

		snprintf(dateStr,20, "%d-%d-%d",d.month,d.day,d.year);
		if (d.min < 10) {
			snprintf(timeStr, 20,"%d : 0%d",d.hour, d.min);
		}
		else {
			snprintf(timeStr, 20,"%d : %d",d.hour, d.min);
		}
		badger.clearToWhite();
		drawClock(DISPLAY_WIDTH/4 - 3*TEXT_PADDING, DISPLAY_HEIGHT/2 - 3*TEXT_PADDING, DISPLAY_HEIGHT*0.4, d.hour, d.min);
		badger.text(dateStr, DISPLAY_WIDTH/2 - 4*TEXT_PADDING, DISPLAY_HEIGHT/4, 0.75f);
		badger.text(timeStr, DISPLAY_WIDTH/2 + 2*TEXT_PADDING, DISPLAY_HEIGHT/2, 0.75f);
		
		if (temp != 0.0f) {
			//badger.text(loc, DISPLAY_WIDTH/2 + 5* TEXT_PADDING, DISPLAY_HEIGHT/2 +TITLE_TEXT_SPACING, TEXT_SIZE);
			//std::string weatherString(50,'\0');
			char weatherString[50];
			snprintf(weatherString, 50, " %.1f F %s" , temp, desc);
			badger.text(weatherString, DISPLAY_WIDTH/3  , DISPLAY_HEIGHT/2 +TITLE_TEXT_SPACING, TEXT_SIZE);
		}
		else {

			badger.text("--F -------", DISPLAY_WIDTH/3  , DISPLAY_HEIGHT/2 +TITLE_TEXT_SPACING, TEXT_SIZE);
		}
		drawSideLabels();
		badger.update();
		int sec = get_seconds_from_datetime_t(d);
		printf("Seconds since epoch: %d\n", sec);
    }
	else {
		LogError(("RTC is not initialized"));

		drawClock(DISPLAY_WIDTH/4, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/3, 6, 0);
		badger.update();
	}

}


void MainView::drawSideLabels(void) {

	//Side screen idx
	for (int i = MAIN_SCREEN; i < NUM_SCREEN; i++) {

		if (screenIdx == i) {
			badger.pen(0);
			badger.thickness(DEFAULT_THICKNESS);
			badger.text(std::to_string(i), DISPLAY_WIDTH - 4*TEXT_PADDING, TEXT_PADDING + (i*TEXT_SPACING), TEXT_SIZE);
		}
		else {
			badger.pen(1);
			badger.thickness(1);
			badger.text(std::to_string(i), DISPLAY_WIDTH - 4*TEXT_PADDING, TEXT_PADDING + (i*TEXT_SPACING), TEXT_SIZE);
		}
	}

	//Button labels
	badger.pen(0);
	badger.thickness(DEFAULT_THICKNESS);
	badger.text("Reminders", TEXT_PADDING, DISPLAY_HEIGHT - (2*TEXT_PADDING), TEXT_SIZE);
	badger.text("Events", DISPLAY_WIDTH/2 - 4*TEXT_PADDING, DISPLAY_HEIGHT - (2*TEXT_PADDING), TEXT_SIZE);
	badger.text("Home", 3*DISPLAY_WIDTH/4 + 4*TEXT_PADDING, DISPLAY_HEIGHT - (2*TEXT_PADDING), TEXT_SIZE);

}

void MainView::updateWeatherInfo(WeatherServiceRequest& req) {
	
	if (req.getWeather("37.76213", "-122.3943")){
		req.getTempValues(temp, tempMin, tempMax);
		req.getDesc(desc);
		req.getLoc(loc);
		LogInfo(("Got weather %s, %s, %.2f, %.2f, %.2f", loc, desc, temp, tempMin, tempMax));
	}
	else {
		LogError(("Failed to get weather"));
	}
}
