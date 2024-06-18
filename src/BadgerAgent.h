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
#include "SwitchObserver.h"
#include "SwitchMgr.h"
#include "tiny-json.h"

#include "pico/stdlib.h"
#include "queue.h"
#include "message_buffer.h"
#include "MQTTConfig.h"
#include "MQTTInterface.h"
#include "badger2040.hpp"
#include <string.h>
#include "timers.h"

using namespace pimoroni;

#define BADGER_QUEUE_LEN 	5
#define MQTT_TOPIC_BADGER_STATE "Badger/state"
#define BADGER_BUFFER_LEN 	512
#define BADGER_JSON_LEN 	512
#define BADGER_JSON_POOL 	10

enum BadgerAction {WriteToScreen, DisplayTime, WriteReminder};

class BadgerAgent : public Agent, public SwitchObserver {
public:
	/***
	 * Constructor
	 * @param ledGP - GPIO Pad of LED to control
	 * @param spstGP - GPIO Pad of SPST non latched switch
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
	 * Toggle LED state from within an intrupt
	 */
	void intToggle();

	/***
	 * Execute the state on the LED and notify MQTT interface
	 * @param state
	 */
	void execLed(bool state);
	void blinkLED(int blinks);


	//Clock and time
	TimerHandle_t clockUpdateTimer;
	int skipTimeDisplayCount = 0; //Used to skip the time display for x Amount of seconds
	void displayTime(void);
	void drawClock(uint8_t x, uint8_t y, uint8_t handLen, uint8_t hour, uint8_t min);

	/***
	 * Parse a JSON string and add request to queue
	 * @param str - JSON Strging
	 */
	void parseJSON(char *str);

	//Interface to publish state to MQTT
	MQTTInterface *pInterface = NULL;

	// Topic to publish on
	char * pTopicBadgerState = NULL;

	//State of the LED
	bool xState = false;

	//String to write to screen
	void writeToDisplay(std::string msg);
	std::string msgToDisplay;

	struct reminder_t {
		std::string title;
		std::string date;
		std::string time;
	}reminder;
	std::vector<reminder_t> reminderVec;
	void writeReminderToDisplay(reminder_t &reminder);

	//LED and switch pads
	Badger2040 badger;
	uint8_t xSpstGP;

	// Switch manage to manage the SPST switch
	SwitchMgr *pSwitchMgr = NULL;
	
	//Variables for wrapping text around display
	int textSpacing;
	int lenPerChar;	
	int charPerLine;

	//Queue of commands
	QueueHandle_t xCmdQ;

	//Timer for blinking led
	TimerHandle_t blinkTimer;
	int blinkCount = 0;
	int numBlinks = 0;

	// Message buffer handle
	MessageBufferHandle_t xBuffer = NULL;

	// Json decoding buffer
	json_t pJsonPool[ BADGER_JSON_POOL ];



};


#endif /* _BADGERAGENT_H_ */
