/*
 * LEDAgent.h
 *
 * Manage LED and latched switch software behaviour
 *
 *  Created on: 23 Oct 2022
 *      Author: jondurrant
 */

#ifndef _BADGERAGENT_H_
#define _BADGERAGENT_H_


#include "Agent.h"
#include "MainView.h"
#include "MessageView.h"
#include "ReminderView.h"
#include "EventView.h"
#include "EventReminder.h"
#include "SwitchObserver.h"
#include "SwitchMgr.h"
#include "tiny-json.h"
#include "NVSOnboard.h"
#include "MusicPlayer.h"

#include "pico/stdlib.h"
#include "queue.h"
#include "message_buffer.h"
#include "MQTTConfig.h"
#include "MQTTInterface.h"
#include "badger2040.hpp"
#include <cstdint>
#include <string.h>
#include "timers.h"
#include <optional>
#include <memory>

using namespace pimoroni;

#define BADGER_QUEUE_LEN 	5
#define MQTT_TOPIC_BADGER_STATE "Badger/state"
#define BADGER_BUFFER_LEN	2048	
#define BADGER_JSON_LEN 	2048
#define BADGER_JSON_POOL 	50


enum BadgerAction { ScrollDown, ScrollUp, RefreshScreen, GetWeather};
enum BadgerButtons{
	UP,
	DOWN,
	A,
	B,
	C,
	NUM_BUTTONS
};

enum JsonFieldsEnum {
	REMINDERS,
	EVENTS,
	MESSAGES,
	NUM_FIELDS
};

union JsonFieldFlag{
	struct{
		unsigned char reminders : 1;
		unsigned char events: 1;
		unsigned char message: 1;
	} b;
	uint8_t u8;
};


class BadgerAgent : public Agent, public SwitchObserver {
public:
	/***
	 * Constructor
	 * @param interface - MQTT Interface that state will be notified to
	 */
	BadgerAgent(MQTTInterface *interface);

	/***
	 * Destructor
	 */
	virtual ~BadgerAgent();


	void sendAction(BadgerAction action, bool putFront=false);

	/***
	 * Toggle the state of the LED. so On becomes Off, etc.
	 */
	void blinkTimerCallback(TimerHandle_t timer);


	/***
	 * Add a JSON string action
	 * @param jsonStr
	 */
	void addJSON(const void  *jsonStr, size_t len);

	/***
	 * Handle a short press from the switch
	 * @param gp - GPIO number of the switch
	 */
	virtual void handleShortPress(uint8_t gp);

	/***
	 * Handle a short press from the switch
	 * @param gp - GPIO number of the switch
	 */
	virtual void handleLongPress(uint8_t gp);

protected:
	/***
	 * Task main run loop
	 */
	virtual void run();

	/***
	 * Get the static depth required in words
	 * @return - words
	 */
	virtual configSTACK_DEPTH_TYPE getMaxStackSize();

private:
	/***
	 * Handle button press from within an interrupt
	 */
	void handleButtonInput(uint8_t gp);

	/***
	 * Execute the state on the LED and notify MQTT interface
	 * @param state
	 */
	void execLed(bool state);
	void blinkLED(int blinks);


	//Clock and time
	TimerHandle_t clockUpdateTimer;
	int skipTimeDisplayCount = 0; //Used to skip the time display for x Amount of seconds

	/***
	 * Parse a JSON string and add request to queue
	 * @param str - JSON Strging
	 */
	void parseJSON(char *str);


	/***
	 * Load and parse a JSON string and add request to queue
	 * @param str - JSON Strging
	 */
	void loadJSONFromNVS(void);
	/***
	 * Parse a JSON string and return bitfield with fields present
	 * @param str - JSON Strging
	 */
	JsonFieldFlag jsonFieldsPresent(char* str, json_t const* outJsons[]);

	//Interface to publish state to MQTT
	MQTTInterface *pInterface = NULL;

	// Topic to publish on
	char * pTopicBadgerState = NULL;

	//State of the LED
	bool xState = false;
	
	//Json methods
	std::optional<eventReminder_t>  processJsonToReminder(json_t const* reminderJson);
	void parseJSONEventsReminders(json_t const* reminderList, json_t const* eventList);
	// Json decoding buffer
	json_t pJsonPool[ BADGER_JSON_POOL ];
	
	//Badger class
	Badger2040 badger;

	// Members and methods to handle button presses
	void handleScrollAction(bool isUp);
	std::array<SwitchMgr*, NUM_BUTTONS> pSwitchMgrs;
	std::array<std::string , NUM_BUTTONS> badgerButtonStringLUT = { "Up", "Down", "A", "B", "C"};
	std::array<int, NUM_BUTTONS> badgerButtonLUT = { Badger2040::UP, Badger2040::DOWN, Badger2040::A, Badger2040::B, Badger2040::C};
	
	std::array<enum BadgerAction, NUM_BUTTONS> badgerButtonActLUT = {ScrollUp, ScrollDown, RefreshScreen, RefreshScreen, RefreshScreen};
	std::map<int, int> badgerButtonToEnum = { 
		{ Badger2040::UP, UP},
		{ Badger2040::DOWN, DOWN},
		{ Badger2040::A, A}, 
		{ Badger2040::B, B},
		{ Badger2040::C, C}};
	
	//Speaker methods and variables
	int BUZZER_GPIO_PIN = 5;
	MusicPlayer player;

	//Queue of commands
	QueueHandle_t xCmdQ;

	//NVS
	NVSOnboard* nvs;

	//Timer for blinking led
	TimerHandle_t blinkTimer;
	int blinkCount = 0;
	int numBlinks = 0;

	// Message buffer handle
	MessageBufferHandle_t xBuffer = NULL;

	//Views
	std::shared_ptr<View> currentView;
	std::shared_ptr<MainView> mainView;
	std::shared_ptr<MessageView> messageView;
	std::shared_ptr<ReminderView> reminderView;
	std::shared_ptr<EventView> eventView;
	
	//View methods
	void refreshDisplay(void);
	void setView(std::shared_ptr<View> view) {
		currentView = view;
	}
	void getWeather(void);
	int weatherUpdateTimer = 0;
	const int weatherUpdateInterval = 10; //Update weather every 10 minutes


};


#endif /* _BADGERAGENT_H_ */
