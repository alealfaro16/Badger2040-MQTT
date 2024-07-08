
#ifndef MAINVIEW_H 
#define MAINVIEW_H

#include "badger2040.hpp"
#include "View.h"
#include "ReminderView.h"
#include "EventView.h"
#include <memory>

using namespace pimoroni;

class MainView : public View {
	
public:
	/***
	 * Constructor
	 * @param interface - MQTT Interface that state will be notified to
	 */
	MainView(Badger2040& badge, int& skipCount, std::shared_ptr<EventView> event, std::shared_ptr<ReminderView> remind) : 
		View(badge), skipTimeCount(skipCount), eventView(event), reminderView(remind) {};
	void displayView(void) override;

	void setScreen(int screen) {
		screen = std::clamp(screen, 0, static_cast<int>(NUM_SCREEN));
		screenIdx = screen;
	}
	
	int getScreen(void) {
		return screenIdx;
	}

	enum {
		INIT_SCREEN,
		MAIN_SCREEN,
		CLOCK_SCREEN,
		NUM_SCREEN
	};
private:
	//Display functions
	void displayInitView(void);
	void displayMainView(void);
	void displayClockView(void);
	
	//Display helper functions
	void drawClock(uint8_t x, uint8_t y, uint8_t handLen, uint8_t hour, uint8_t min);
	void drawSideLabels(void);

	std::shared_ptr<EventView> eventView;
	std::shared_ptr<ReminderView> reminderView;
	int& skipTimeCount;

	typedef void (MainView::*displayFunction)();
	displayFunction displayFuncs[NUM_SCREEN] = {
		&MainView::displayInitView,
		&MainView::displayMainView,
		&MainView::displayClockView
	};

	int screenIdx = INIT_SCREEN;
};

#endif
