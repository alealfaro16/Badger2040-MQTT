/*
 * BadgerAgent.cpp
 *
 * Manage LED and latched switch software behaviour
 *
 *  Created on: 23 Oct 2022
 *      Author: jondurrant
 */

#include "BadgerAgent.h"
#include "MQTTTopicHelper.h"
#include "SwitchMgr.h"
#include "badger2040.hpp"
#include "hardware/rtc.h"
#include "logging_stack.h"
#include "pico/types.h"
#include "projdefs.h"
#include "tiny-json.h"
#include <cstdint>
#include <string.h>
#include <math.h>
#include <stdexcept>

//Local enumerator of the actions to be queued

const int DISPLAY_WIDTH = 340; //Needs tuning
const int DISPLAY_HEIGHT = 128;
#define TEXT_PADDING  4
constexpr float TEXT_SIZE = 0.5f;
constexpr float REMINDER_TEXT_SIZE = 0.7f;
#define TEXT_WIDTH  (DISPLAY_WIDTH - TEXT_PADDING - TEXT_PADDING)
#define NUM_BLINKS_MESSAGE 10
/***
 * Constructor
 * @param ledGP - GPIO Pad of LED to control
 * @param spstGP - GPIO Pad of SPST non latched switch
 * @param interface - MQTT Interface that state will be notified to
 */
BadgerAgent::BadgerAgent(MQTTInterface *interface) {

	badger.init();
	pInterface = interface;

	//Initialize variables to write text in display
	textSpacing = 34*TEXT_SIZE;
	int lenPerChar = badger.measure_text("a", TEXT_SIZE);
	charPerLine = TEXT_WIDTH/lenPerChar;
	LogDebug(("len per char: %d and charPerLine %d", lenPerChar, charPerLine));
	badger.pen(15);
	badger.clear();
	badger.pen(0);
	badger.font("serif");
	badger.thickness(2);
	badger.text("Initiating Badger....", TEXT_PADDING, textSpacing + TEXT_PADDING, TEXT_SIZE);
	badger.update();
	//Construct switch observer and listen
	for (int i = 0; i < BADGER_BUTTON_USED; i++) {
		pSwitchMgrs[i] = new SwitchMgr(badgerButtonLUT[i], false);
		pSwitchMgrs[i]->setObserver(this);
	}

	// Queue for actions commands for the class
	xCmdQ = xQueueCreate( BADGER_QUEUE_LEN, sizeof(BadgerAction));
	if (xCmdQ == NULL){
		LogError(("Unable to create Queue\n"));
	}

	//Construct a message buffer
	xBuffer = xMessageBufferCreate(BADGER_BUFFER_LEN);
	if (xBuffer == NULL){
		LogError(("Buffer could not be allocated\n"));
	}

	//Construct the TOPIC for status messages
	if (pInterface != NULL){
		if (pTopicBadgerState == NULL){
			pTopicBadgerState = (char *)pvPortMalloc( MQTTTopicHelper::lenThingTopic(pInterface->getId(), MQTT_TOPIC_BADGER_STATE));
			if (pTopicBadgerState != NULL){
				MQTTTopicHelper::genThingTopic(pTopicBadgerState, pInterface->getId(), MQTT_TOPIC_BADGER_STATE);
			} else {
				LogError( ("Unable to allocate topic") );
			}
			LogInfo(("Topic to publish badger state: %s", pTopicBadgerState));
		}
	}

	//Create timer for blinking LED
	auto blinkTimerCallback = [](TimerHandle_t timer) {
		BadgerAgent* agent = static_cast<BadgerAgent*>(pvTimerGetTimerID(timer));
		assert(agent);
		if (agent->blinkCount > agent->numBlinks) {
			xTimerStop(agent->blinkTimer, 0);
			agent->execLed(false);
		}
		else {
			agent->blinkTimerCallback(timer);
		}
	};
	blinkTimer = xTimerCreate("Blink timer", pdMS_TO_TICKS(500), pdTRUE, static_cast<void*>(this), blinkTimerCallback);
	blinkLED(5);

	//Create timer clock update
	auto clockUpdateCallback = [](TimerHandle_t timer) {
		BadgerAgent* agent = static_cast<BadgerAgent*>(pvTimerGetTimerID(timer));
		assert(agent);
		agent->sendAction(DisplayTime);
	};
	clockUpdateTimer = xTimerCreate("Clock timer", pdMS_TO_TICKS(1000*60), pdTRUE, static_cast<void*>(this), clockUpdateCallback);
	if( xTimerStart( clockUpdateTimer, 0 ) != pdPASS )
	{
		/* The timer could not be set into the Active
				 state. */
		LogError(("Clock update timer couldn't be started"));
	}

}

/***
 * Destructor
 */
BadgerAgent::~BadgerAgent() {
	
	for (auto mgr : pSwitchMgrs) {
		if (mgr != NULL){
			delete mgr;
		}
	}
	if (xCmdQ != NULL){
		vQueueDelete(xCmdQ);
	}
	if (pTopicBadgerState != NULL){
		vPortFree(pTopicBadgerState);
		pTopicBadgerState = NULL;
	}
	if (xBuffer != NULL){
		vMessageBufferDelete(xBuffer);
	}
}


/***
 * Handle a short press from the switch
 * @param gp - GPIO number of the switch
 */
void BadgerAgent::handleShortPress(uint8_t gp){
	handleButtonInput(gp);
}

/***
 * Handle a short press from the switch
 * @param gp - GPIO number of the switch
 */
void BadgerAgent::handleLongPress(uint8_t gp){
	handleButtonInput(gp);
}

void BadgerAgent::handleScrollAction(bool isUp) {
	
	//Nothing to do if there's no reminders
	if (reminderVec.empty()) return;
	int prev = reminderIdx;
	isUp ? reminderIdx-- : reminderIdx++;
	reminderIdx = std::clamp(reminderIdx, 0, static_cast<int>(reminderVec.size() - 1));
	
	//No change to index so don't need to refresh screen
	if (prev == reminderIdx) return;
	
	LogInfo(("New reminder idx: %d", reminderIdx));
	sendAction(WriteReminder, true);
}

void BadgerAgent::sendAction(BadgerAction action, bool putFront){
	BaseType_t res;
	if (putFront) {
		res = xQueueSendToFront(xCmdQ, (void *)&action, 0);
	}
	else {
		res = xQueueSendToBack(xCmdQ, (void *)&action, 0);
	}
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}

/***
 * Toggle LED state from within an intrupt
 */
void BadgerAgent::handleButtonInput(uint8_t gp){
	auto idx = badgerButtonToEnum.find(gp);	
	if (idx ==  badgerButtonToEnum.end()){
		LogError(("Unavailable button got pressed : %d", gp));
		return;
	}
	
	enum BadgerButtons buttonType = static_cast<enum BadgerButtons>(idx->second);
	LogInfo(("Got %s button press", badgerButtonStringLUT[buttonType].c_str()));
	BadgerAction action = badgerButtonActLUT[buttonType];
	BaseType_t res = xQueueSendToFrontFromISR(xCmdQ, (void *)&action, NULL);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}


/***
  * Main Run Task for agent
  */
void BadgerAgent::run(){
	BaseType_t res;
	BadgerAction action = WriteToScreen;
	char jsonStr[BADGER_JSON_LEN];
	size_t readLen;

	if (xCmdQ == NULL){
		return;
	}

	while (true) { // Loop forever
		readLen = xMessageBufferReceive(
						 xBuffer,
				         jsonStr,
						 BADGER_JSON_LEN,
						 0
				    );
		if (readLen > 0){
			jsonStr[readLen] = 0;
	        parseJSON(jsonStr);
		}

		res = xQueueReceive(xCmdQ, (void *)&action, 0);
		if (res == pdTRUE){
			switch(action){
				case WriteReminder:{
					writeReminderToDisplay();
					xQueueReset(xCmdQ);//Delete other commands in queue to prevent overwrite of message
					break;
				}
				case WriteToScreen:{
					writeToDisplay(msgToDisplay);
					xQueueReset(xCmdQ);//Delete other commands in queue to prevent overwrite of message
					break;
				}
				case DisplayTime:{
					displayTime();
					break;
				}
				case DisplayReminders: {
					break;
				}
				case DisplayEvents: {
					displayEvents();
					break;
				}
				case ScrollDown: {
					handleScrollAction(false);
					break;
				}
				case ScrollUp: {
					handleScrollAction(true);
					break;
				}
			}
			taskYIELD();
		}
	}
}


/***
 * Execute the state on the LED and notify MQTT interface
 * @param state
 */
void BadgerAgent::execLed(bool state){
	xState = state;
	int pwmVal = state ? 255 : 0;
	badger.led(pwmVal);	
}


void BadgerAgent::blinkTimerCallback(TimerHandle_t timer) {
	xState = !xState;
	blinkCount++;
	execLed(xState);
}

void BadgerAgent::blinkLED(int blinks){
	numBlinks = blinks;
	blinkCount = 0;
	if( xTimerStart( blinkTimer, 0 ) != pdPASS )
	{
		/* The timer could not be set into the Active
				 state. */
		LogError(("Blink timer couldn't be started"));
	}

}


void BadgerAgent::displayTime(void) {
	
	//Counter to skip the time display for X seconds
	if (skipTimeDisplayCount > 0) {
		skipTimeDisplayCount--;
		return;
	}

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
	}
}

void BadgerAgent::displayEvents(void) {
	badger.pen(15);
	badger.clear();
	badger.pen(0);
	datetime_t d;
	rtc_get_datetime(&d);
	std::string titleText = "Events (" + std::to_string(d.month) + "/" + std::to_string(d.day) + ")";
	badger.text(titleText, 10, 10, REMINDER_TEXT_SIZE);
	int titleTextSpacing = 34*REMINDER_TEXT_SIZE;
	int yTopMargin = 10 + titleTextSpacing;
	int row = 0;
	for (auto& reminder : reminderVec) {
		if(reminder.type == reminder_t::EVENT) {
			badger.text(reminder.time, DISPLAY_WIDTH/3, (row++)*textSpacing + yTopMargin ,TEXT_SIZE);
			badger.text(reminder.title, TEXT_PADDING, (row++)*textSpacing + yTopMargin, TEXT_SIZE);
		}

		if (row*textSpacing > DISPLAY_HEIGHT) break;
	}
	badger.update();
}


void BadgerAgent::drawClock(uint8_t x, uint8_t y, uint8_t handLen, uint8_t hour, uint8_t min){
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

void BadgerAgent::writeToDisplay(std::string msg){
	
	//Blink LED to show new message is being written
	blinkLED(NUM_BLINKS_MESSAGE);

	skipTimeDisplayCount = 2; //Skip clock display for 2 minutes
	
	badger.pen(15);
	badger.clear();
	badger.pen(0);

	//LogInfo(("msg received: %s",msg.c_str()));
	int len = badger.measure_text(msg, TEXT_SIZE);
	int rows = (len / TEXT_WIDTH) + 1;
	LogDebug(("string length is %d and it's display length is: %d", msg.length(),len));
	LogDebug(("rows to print: %d",rows));
	for (int r = 0; r < rows; r++) {
		std::string line  = msg.substr(r*charPerLine, charPerLine);
		LogDebug(("line: %s", line.c_str()));
		badger.text(line, TEXT_PADDING, r*textSpacing + 10, TEXT_SIZE);
	}
	badger.update();

}

void BadgerAgent::writeReminderToDisplay(){
	
	try {
		reminder_t& reminder = reminderVec.at(reminderIdx);
		//Blink LED to show new message is being written
		blinkLED(NUM_BLINKS_MESSAGE);

		skipTimeDisplayCount = 2; //Skip clock display for 2 minutes
		//
		int lenChar = badger.measure_text("A", REMINDER_TEXT_SIZE);
		int cPerLine = TEXT_WIDTH/lenChar;
		int textSpacing = 34*REMINDER_TEXT_SIZE;

		badger.pen(15);
		badger.clear();
		badger.pen(0);

		int len = badger.measure_text(reminder.title, REMINDER_TEXT_SIZE);
		int rows = (len / TEXT_WIDTH) + 1;
		LogDebug(("Reminder title length is %d and it's display length is: %d", tile.length(),len));
		LogDebug(("rows to print: %d",rows));
		badger.text(reminder.stringLUT[reminder.type], 10, 10, REMINDER_TEXT_SIZE);
		for (int r = 0; r < rows; r++) {
			std::string line  = reminder.title.substr(r*cPerLine, cPerLine);
			LogDebug(("line: %s", line.c_str()));
			badger.text(line, TEXT_PADDING, r*textSpacing + 10 + textSpacing, REMINDER_TEXT_SIZE);
		}

		badger.text(reminder.time, TEXT_PADDING, 3*DISPLAY_HEIGHT/4, REMINDER_TEXT_SIZE);
		badger.text(reminder.date, TEXT_PADDING, 7*DISPLAY_HEIGHT/8, REMINDER_TEXT_SIZE);

		badger.update();

	}
	catch (const std::out_of_range& oor) {
		LogError(("Reminder idx is out of bounds of reminder vec!"));
	}


}


/***
 * Get the static depth required in words
 * @return - words
 */
 configSTACK_DEPTH_TYPE BadgerAgent::getMaxStackSize(){
	 return 1024;
 }

std::optional<BadgerAgent::reminder_t> BadgerAgent::processJsonToReminder(json_t const* reminderJson, reminder_t::Type type) {

	char const* titleJ = json_getPropertyValue( reminderJson, "title" );
	char const* dateJ = json_getPropertyValue( reminderJson, "date" );
	char const* timeJ = json_getPropertyValue( reminderJson, "time" );
	if ( titleJ  && dateJ && timeJ) {
		reminder_t newReminder = { .title = titleJ,
			.date = dateJ,
			.time = timeJ,
			.type = type};
		LogDebug(("Reminder: %s , due date: %s %s",newReminder.title.c_str(), newReminder.time.c_str(), newReminder.date.c_str()));
		return newReminder;
	}
	return std::nullopt;
}


void BadgerAgent::parseJSONEventsReminders(json_t const* reminderList, json_t const* eventList) {

	reminderVec.clear();
	if (reminderList != NULL) {
		for( auto remind = json_getChild( reminderList ); remind != 0; remind = json_getSibling( remind ) ) {
			if ( JSON_OBJ == json_getType( remind ) ) {
				auto newReminder = processJsonToReminder(remind, reminder_t::REMINDER);
				if (newReminder.has_value()) reminderVec.push_back(newReminder.value());
			}
			else {
				LogError(("Couldn't parse reminder!"));
			}
		}
	}
	
	if (eventList != NULL) {
		for( auto event = json_getChild( eventList ); event != 0; event = json_getSibling( event ) ) {
			if ( JSON_OBJ == json_getType( event ) ) {
				auto newEvent = processJsonToReminder(event, reminder_t::EVENT);
				if (newEvent.has_value()) reminderVec.push_back(newEvent.value());
			}
			else {
				LogError(("Couldn't parse reminder!"));
			}
		}
	}
	
	//Start at the top of reminder vec
	reminderIdx = 0;
}

/***
* Parse a JSON string and add request to queue
* @param str - JSON Strging
*/
void BadgerAgent::parseJSON(char *str){
	json_t const* json = json_create( str, pJsonPool, BADGER_JSON_POOL);
	if ( !json ) {
		LogError(("Error json create."));
		return ;
	}
	
	
	bool remindersReceived = false, eventsReceived = false;
	json_t const* remindersJson = json_getProperty( json, "reminders" );
	if ( !remindersJson|| JSON_ARRAY != json_getType( remindersJson) ) {
		LogInfo(("The reminders property is not found."));

	}
	else {
		remindersReceived = true;
	}

	json_t const* eventJson = json_getProperty( json, "calendar" );
	if ( !eventJson|| JSON_ARRAY != json_getType( eventJson) ) {
		LogInfo(("The calendar property is not found."));
	}
	else {
		eventsReceived = true;
	}

	if (remindersReceived || eventsReceived) {
		parseJSONEventsReminders(remindersReceived ? remindersJson : NULL, eventsReceived ? eventJson : NULL);
		sendAction(WriteReminder, true);//Put this action on the front of the queue
	}


	json_t const* message = json_getProperty( json, "message" );
	if ( !message || JSON_TEXT != json_getType( message ) ) {
		LogInfo(("The message property is not found."));
	}
	else {
		msgToDisplay = json_getValue(message);
		sendAction(WriteToScreen, true);//Put this action on the front of the queue
	}

}


/***
 * Add a JSON string action
 * @param jsonStr
 */
void BadgerAgent::addJSON(const void  *jsonStr, size_t len){

	if (len >= BADGER_BUFFER_LEN) {
		LogError(("Message received is longer than the buffer len %d", BADGER_BUFFER_LEN));
		return;
	}

	if (xBuffer != NULL){
		size_t res = xMessageBufferSend(
			xBuffer,
			jsonStr,
			len,
			0);

		if (res != len){
			LogError(("Failed to write"));
		}
	}
}
