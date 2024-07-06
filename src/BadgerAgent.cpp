/*
 * BadgerAgent.cpp
 *
 * Manage LED and latched switch software behaviour
 *
 *  Created on: 23 Oct 2022
 *      Author: jondurrant
 */

#include "BadgerAgent.h"
#include "EventReminder.h"
#include "EventView.h"
#include "MQTTTopicHelper.h"
#include "MessageView.h"
#include "ReminderView.h"
#include "SwitchMgr.h"
#include "badger2040.hpp"
#include "hardware/rtc.h"
#include "logging_stack.h"
#include "pico/types.h"
#include "projdefs.h"
#include "tiny-json.h"
#include <cstdint>
#include <ctime>
#include <string.h>
#include <math.h>
#include <stdexcept>
#include <memory.h>

//Local enumerator of the actions to be queued

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
	
	//Initialize views
	currentView = std::shared_ptr<View>(new View(badger));
	timeView = std::shared_ptr<TimeView>(new TimeView(badger, skipTimeDisplayCount));
	messageView = std::shared_ptr<MessageView>(new MessageView(badger, skipTimeDisplayCount));
	reminderView = std::shared_ptr<ReminderView>(new ReminderView(badger, skipTimeDisplayCount));
	eventView = std::shared_ptr<EventView>(new EventView(badger, skipTimeDisplayCount));
	currentView->displayView();

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
		//Counter to skip the time display for X seconds
		if (agent->skipTimeDisplayCount > 0) {
			agent->skipTimeDisplayCount--;
			return;
		}
		agent->setView(agent->timeView);
		agent->sendAction(RefreshScreen);
	};
	clockUpdateTimer = xTimerCreate("Clock timer", pdMS_TO_TICKS(1000*60), pdTRUE, static_cast<void*>(this), clockUpdateCallback);
	if( xTimerStart( clockUpdateTimer, 0 ) != pdPASS )
	{
		/* The timer could not be set into the Active
				 state. */
		LogError(("Clock update timer couldn't be started"));
	}

	//Initialize NVS
	nvs = NVSOnboard::getInstance();
	if (nvs->contains("reminders")) {
		int maxLen = BADGER_JSON_LEN;
		char reminderJson[maxLen];
		nvs->get_str("reminders", reminderJson, &maxLen);
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
	
	//Check what screen is currently displayed
	if (ReminderView* rView = dynamic_cast<ReminderView*>(currentView.get())) {
		//Nothing to do if there's no reminders
		if (reminderView->isEmpty()) return;
		int idx = reminderView->getIndex();
		isUp ? idx-- : idx++;

		//Check if the index is the same as before. If so don't refresh screen
		if (reminderView->setIndex(idx) == false) return; 

		LogInfo(("New reminder idx: %d", idx));
		sendAction(RefreshScreen, true);
    }
	else if (EventView* eView = dynamic_cast<EventView*>(currentView.get())) {
		eventView->scrollUpdate(isUp);
		sendAction(RefreshScreen, true);
	}
	
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
	if (buttonType == A) {
		//Set view
		setView(reminderView);
	}
	else if (buttonType == B) {
		//Set view
		setView(eventView);
	}
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
	BadgerAction action = RefreshScreen;
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
				case ScrollDown: {
					handleScrollAction(false);
					break;
				}
				case ScrollUp: {
					handleScrollAction(true);
					break;
				}
				case RefreshScreen: {
					refreshDisplay();
					break;
				}
				case SAVE_TO_NVS: {

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



/***
 * Get the static depth required in words
 * @return - words
 */
 configSTACK_DEPTH_TYPE BadgerAgent::getMaxStackSize(){
	 return 1024;
 }

std::optional<eventReminder_t> BadgerAgent::processJsonToReminder(json_t const* reminderJson) {

	char const* titleJ = json_getPropertyValue( reminderJson, "title" );
	char const* dateJ = json_getPropertyValue( reminderJson, "date" );
	char const* timeJ = json_getPropertyValue( reminderJson, "time" );
	if ( titleJ  && dateJ && timeJ) {
		eventReminder_t newEventReminder = { 
			.title = titleJ,
			.date = dateJ,
			.time = timeJ}
		LogDebug(("Reminder: %s , due date: %s %s",newEventReminder.title.c_str(), newEventReminder.time.c_str(), newEventReminder.date.c_str()));
		return newEventReminder;
	}
	return std::nullopt;
}

void BadgerAgent::parseJSONEventsReminders(json_t const* reminderList, json_t const* eventList) {

	//Clear previous reminders and start at index 0
	reminderView->clear();
	if (reminderList != NULL) {
		for( auto remind = json_getChild( reminderList ); remind != 0; remind = json_getSibling( remind ) ) {
			if ( JSON_OBJ == json_getType( remind ) ) {
				auto newReminder = processJsonToReminder(remind);
				if (newReminder.has_value()) reminderView->addReminder(newReminder.value());
			}
			else {
				LogError(("Couldn't parse reminder!"));
			}
		}
	}
	
	if (eventList != NULL) {
		for( auto event = json_getChild( eventList ); event != 0; event = json_getSibling( event ) ) {
			if ( JSON_OBJ == json_getType( event ) ) {
				auto newEvent = processJsonToReminder(event);
				if (newEvent.has_value()) eventView->addEvent(newEvent.value());
			}
			else {
				LogError(("Couldn't parse reminder!"));
			}
		}
	}
	
}

/***
* Parse a JSON string and add request to queue
* @param str - JSON Strging
*/
void BadgerAgent::parseJSON(char *str){

	std::string sCopy(str);
	json_t const* json = json_create( str, pJsonPool, BADGER_JSON_POOL);
	if ( !json ) {
		LogError(("Error json create."));
		return ;
	}
	
	
	bool remindersReceived = false, eventsReceived = false;
	json_t const* remindersJson = json_getProperty( json, "reminders" );
	if ( !remindersJson|| JSON_ARRAY != json_getType( remindersJson) ) {
		LogDebug(("The reminders property is not found."));

	}
	else {
		remindersReceived = true;
	}

	json_t const* eventJson = json_getProperty( json, "calendar" );
	if ( !eventJson|| JSON_ARRAY != json_getType( eventJson) ) {
		LogDebug(("The calendar property is not found."));
	}
	else {
		eventsReceived = true;
	}

	if (remindersReceived || eventsReceived) {
		parseJSONEventsReminders(remindersReceived ? remindersJson : NULL, eventsReceived ? eventJson : NULL);
		messageView->setMessage("New events and reminders. Press A to see reminder and B to see events");
		setView(messageView);
		blinkLED(NUM_BLINKS_MESSAGE);
		sendAction(RefreshScreen, true);
		if (remindersReceived) {
			nvs->set_str("reminders", sCopy.c_str());
			nvs->commit();
		}
	}


	json_t const* message = json_getProperty( json, "message" );
	if ( !message || JSON_TEXT != json_getType( message ) ) {
		LogDebug(("The message property is not found."));
	}
	else {
		std::string msgToDisplay = json_getValue(message);
		messageView->setMessage(msgToDisplay);
		setView(messageView);
		blinkLED(NUM_BLINKS_MESSAGE);
		sendAction(RefreshScreen, true);
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


void BadgerAgent::refreshDisplay(void) {
	currentView->displayView();
}
