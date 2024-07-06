#ifndef TIMEVIEW_H
#define TIMEVIEW_H

#include "badger2040.hpp"
#include "View.h"

using namespace pimoroni;

class TimeView : public View {
	
public:
	/***
	 * Constructor
	 * @param interface - MQTT Interface that state will be notified to
	 */
	TimeView(Badger2040& badge, int& skipCount) : View(badge), skipTimeCount(skipCount) {};
	void drawClock(uint8_t x, uint8_t y, uint8_t handLen, uint8_t hour, uint8_t min);
	void displayView(void) override;

	int& skipTimeCount;

};

#endif
