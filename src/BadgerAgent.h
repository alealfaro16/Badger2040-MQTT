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
#define BADGER_BUTTON_USED  4


enum BadgerAction { ScrollDown, ScrollUp, RefreshScreen, PlaySong};
enum BadgerButtons{
	UP,
	DOWN,
	A,
	B
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
	std::array<SwitchMgr*, BADGER_BUTTON_USED> pSwitchMgrs;
	std::array<int, BADGER_BUTTON_USED> badgerButtonLUT = { Badger2040::UP, Badger2040::DOWN, Badger2040::A, Badger2040::B};
	std::array<enum BadgerAction, BADGER_BUTTON_USED> badgerButtonActLUT = {ScrollUp, ScrollDown, RefreshScreen, RefreshScreen};
	std::array<std::string , BADGER_BUTTON_USED> badgerButtonStringLUT = { "Up", "Down", "A", "B"};
	std::map<int, int> badgerButtonToEnum = { 
		{ Badger2040::UP, UP},
		{ Badger2040::DOWN, DOWN},
		{ Badger2040::A, A}, 
		{ Badger2040::B, B}};
	
	//Speaker methods and variables
	TimerHandle_t songTimer;
	int toneIndex = 0;
	int tempo = 180;
	int wholeNote;
	int toneNum;
	int BUZZER_GPIO_PIN = 5;
	int pwmSlice;
	int pwmChan;
	void silenceSpeaker(void);
	void playTone(int freq);
	void playSong(void);
	static void songTaskFunc(void* params);
	std::vector<std::string> initSong = {"E5","G5","A5","P","E5","G5","B5","A5","P","E5","G5","A5","P","G5","E5"};
	std::map<std::string, uint16_t> toneMap = {
		{"B0", 31},
		{"C1", 33},
		{"CS1", 35},
		{"D1", 37},
		{"DS1", 39},
		{"E1", 41},
		{"F1", 44},
		{"FS1", 46},
		{"G1", 49},
		{"GS1", 52},
		{"A1", 55},
		{"AS1", 58},
		{"B1", 62},
		{"C2", 65},
		{"CS2", 69},
		{"D2", 73},
		{"DS2", 78},
		{"E2", 82},
		{"F2", 87},
		{"FS2", 93},
		{"G2", 98},
		{"GS2", 104},
		{"A2", 110},
		{"AS2", 117},
		{"B2", 123},
		{"C3", 131},
		{"CS3", 139},
		{"D3", 147},
		{"DS3", 156},
		{"E3", 165},
		{"F3", 175},
		{"FS3", 185},
		{"G3", 196},
		{"GS3", 208},
		{"A3", 220},
		{"AS3", 233},
		{"B3", 247},
		{"C4", 262},
		{"CS4", 277},
		{"D4", 294},
		{"DS4", 311},
		{"E4", 330},
		{"F4", 349},
		{"FS4", 370},
		{"G4", 392},
		{"GS4", 415},
		{"A4", 440},
		{"AS4", 466},
		{"B4", 494},
		{"C5", 523},
		{"CS5", 554},
		{"D5", 587},
		{"DS5", 622},
		{"E5", 659},
		{"F5", 698},
		{"FS5", 740},
		{"G5", 784},
		{"GS5", 831},
		{"A5", 880},
		{"AS5", 932},
		{"B5", 988},
		{"C6", 1047},
		{"CS6", 1109},
		{"D6", 1175},
		{"DS6", 1245},
		{"E6", 1319},
		{"F6", 1397},
		{"FS6", 1480},
		{"G6", 1568},
		{"GS6", 1661},
		{"A6", 1760},
		{"AS6", 1865},
		{"B6", 1976},
		{"C7", 2093},
		{"CS7", 2217},
		{"D7", 2349},
		{"DS7", 2489},
		{"E7", 2637},
		{"F7", 2794},
		{"FS7", 2960},
		{"G7", 3136},
		{"GS7", 3322},
		{"A7", 3520},
		{"AS7", 3729},
		{"B7", 3951},
		{"C8", 4186},
		{"CS8", 4435},
		{"D8", 4699},
		{"DS8", 4978}
	};

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


};


#endif /* _BADGERAGENT_H_ */
