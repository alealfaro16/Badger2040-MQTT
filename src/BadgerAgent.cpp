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
#include "hardware/pwm.h"
#include "logging_stack.h"
#include "pico/time.h"
#include "pico/types.h"
#include "projdefs.h"
#include "tiny-json.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string.h>
#include <math.h>
#include <stdexcept>
#include <memory.h>
#include <bit>
#include "MusicDefs.h"

//Local enumerator of the actions to be queued

#define NUM_BLINKS_MESSAGE 10
#define SONG_TASK_PRIORITY			( tskIDLE_PRIORITY + 1UL )

uint32_t pwm_set_freq_duty(uint slice_num,
       uint chan,uint32_t f, int d)
{
	uint32_t clock = 125000000;
	uint32_t divider16 = clock / f / 4096 + 
		(clock % (f * 4096) != 0);
	if (divider16 / 16 == 0)
		divider16 = 16;
	uint32_t wrap = clock * 16 / divider16 / f - 1;
	pwm_set_clkdiv_int_frac(slice_num, divider16/16,
						 divider16 & 0xF);
	pwm_set_wrap(slice_num, wrap);
	pwm_set_chan_level(slice_num, chan, wrap * d / 100);
	return wrap;
}

void BadgerAgent::playTone(int freq) {
	pwm_set_freq_duty(pwmSlice, pwmChan, freq, 75);
}

void BadgerAgent::silenceSpeaker(void) {
	pwm_set_chan_level(pwmSlice, pwmChan, 0);
}

void BadgerAgent::playSong(void) {

	//std::string tone = initSong[toneIndex];
	if (toneIndex == toneNum) {
		xTimerStop(songTimer, 0);
		toneIndex = 0;
		silenceSpeaker();
		return;
	}
	int divider = nokiaMelody[2*toneIndex + 1];
	int noteDuration = 0;
	if (divider > 0) {
		// regular note, just proceed
		noteDuration = (wholeNote) / divider;
	} else if (divider < 0) {
		// dotted notes are represented with negative durations!!
		noteDuration = (wholeNote) / abs(divider);
		noteDuration *= 1.5; // increases the duration in half for dotted notes
	}

	if (nokiaMelody[2*toneIndex] == 0) silenceSpeaker();
	else {
		//assert(toneMap.find(tone) != toneMap.end());
		playTone(nokiaMelody[2*toneIndex]);
	}
	if( xTimerChangePeriod( songTimer, noteDuration/ portTICK_PERIOD_MS, 100 )
                                                            != pdPASS ) {
		LogError(("Couldn't change the period of the timer!"));
	}

	toneIndex++;
}


/***
 * Constructor
 * @param ledGP - GPIO Pad of LED to control
 * @param spstGP - GPIO Pad of SPST non latched switch
 * @param interface - MQTT Interface that state will be notified to
 */
BadgerAgent::BadgerAgent(MQTTInterface *interface) : currentView(NULL), pwmSlice(pwm_gpio_to_slice_num(BUZZER_GPIO_PIN)), pwmChan(pwm_gpio_to_channel(BUZZER_GPIO_PIN)){

	badger.init();
	pInterface = interface;
	
	//Initialize views
	messageView = std::shared_ptr<MessageView>(new MessageView(badger, skipTimeDisplayCount));
	reminderView = std::shared_ptr<ReminderView>(new ReminderView(badger, skipTimeDisplayCount));
	eventView = std::shared_ptr<EventView>(new EventView(badger, skipTimeDisplayCount));
	mainView = std::shared_ptr<MainView>(new MainView(badger, skipTimeDisplayCount, eventView, reminderView));

	//Display init screen
	setView(mainView);
	currentView->displayView();

	//Construct switch observer and listen
	for (int i = 0; i < BADGER_BUTTON_USED; i++) {
		pSwitchMgrs[i] = new SwitchMgr(badgerButtonLUT[i], false);
		pSwitchMgrs[i]->setObserver(this);
	}

	//Init PWM for buzzer
	gpio_set_function(BUZZER_GPIO_PIN, GPIO_FUNC_PWM);
	uint32_t wrap =  pwm_set_freq_duty(pwmSlice, pwmChan, toneMap["B2"], 100);
	pwm_set_enabled(pwmSlice, true);

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
		agent->setView(agent->mainView);
		agent->mainView->setScreen(MainView::CLOCK_SCREEN);
		agent->sendAction(RefreshScreen);
	};
	clockUpdateTimer = xTimerCreate("Clock timer", pdMS_TO_TICKS(1000*60), pdTRUE, static_cast<void*>(this), clockUpdateCallback);
	if( xTimerStart( clockUpdateTimer, 0 ) != pdPASS )
	{
		/* The timer could not be set into the Active
				 state. */
		LogError(("Clock update timer couldn't be started"));
	}

	//Initialize NVS and load json in memory
	nvs = NVSOnboard::getInstance();
	loadJSONFromNVS();
	
	//Set to main screen
	mainView->setScreen(MainView::MAIN_SCREEN);
	
	//Define the duration of a whole note in ms and calculate the number of tones in the song
	wholeNote = (60000*4)/tempo;
	// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
	// there are two values per note (pitch and duration), so for each note there are four bytes
	toneNum = sizeof(nokiaMelody)/sizeof(nokiaMelody[0])/2;
	//Create timer to play song
	auto songTimerCallback = [](TimerHandle_t timer) {
		BadgerAgent* agent = static_cast<BadgerAgent*>(pvTimerGetTimerID(timer));
		assert(agent);
		agent->playSong();
	};
	songTimer = xTimerCreate("Song timer", pdMS_TO_TICKS(300), pdTRUE, static_cast<void*>(this), songTimerCallback);
	if( xTimerStart( songTimer, 0 ) != pdPASS )
	{
		/* The timer could not be set into the Active
				 state. */
		LogError(("Song timer couldn't be started"));
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
	else if (MainView* main = dynamic_cast<MainView*>(currentView.get())) {
		int screenIdx = mainView->getScreen();
		isUp ? screenIdx-- : screenIdx++;
		screenIdx = std::clamp(screenIdx, static_cast<int>(MainView::MAIN_SCREEN), static_cast<int>(MainView::NUM_SCREEN));
		if (screenIdx == mainView->getScreen()) return;
		
		//Set new screen
		mainView->setScreen(screenIdx);
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
			.time = timeJ};
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
	
	eventView->clear();
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
//void BadgerAgent::parseJSON(char *str){
//
//	std::string sCopy(str);
//	json_t const* json = json_create( str, pJsonPool, BADGER_JSON_POOL);
//	if ( !json ) {
//		LogError(("Error json create."));
//		return ;
//	}
//	
//	
//	bool remindersReceived = false, eventsReceived = false;
//	json_t const* remindersJson = json_getProperty( json, "reminders" );
//	if ( !remindersJson|| JSON_ARRAY != json_getType( remindersJson) ) {
//		LogDebug(("The reminders property is not found."));
//
//	}
//	else {
//		remindersReceived = true;
//	}
//
//	json_t const* eventJson = json_getProperty( json, "calendar" );
//	if ( !eventJson|| JSON_ARRAY != json_getType( eventJson) ) {
//		LogDebug(("The calendar property is not found."));
//	}
//	else {
//		eventsReceived = true;
//	}
//
//	if (remindersReceived || eventsReceived) {
//		parseJSONEventsReminders(remindersReceived ? remindersJson : NULL, eventsReceived ? eventJson : NULL);
//		messageView->setMessage("New events and reminders. Press A to see reminder and B to see events");
//		setView(messageView);
//		blinkLED(NUM_BLINKS_MESSAGE);
//		sendAction(RefreshScreen, true);
//		if (remindersReceived) {
//			nvs->set_str("reminders", sCopy.c_str());
//			nvs->commit();
//		}
//	}
//
//
//	json_t const* message = json_getProperty( json, "message" );
//	if ( !message || JSON_TEXT != json_getType( message ) ) {
//		LogDebug(("The message property is not found."));
//	}
//	else {
//		std::string msgToDisplay = json_getValue(message);
//		messageView->setMessage(msgToDisplay);
//		setView(messageView);
//		blinkLED(NUM_BLINKS_MESSAGE);
//		sendAction(RefreshScreen, true);
//	}
//
//}

/***
* Parse a JSON string and add request to queue
* @param str - JSON Strging
*/
void BadgerAgent::parseJSON(char *str){

	json_t const* subJsons[NUM_FIELDS];
	std::string cleanStr(str);
	JsonFieldFlag fields = jsonFieldsPresent(str, subJsons);

	if (fields.b.events|| fields.b.reminders) {
		parseJSONEventsReminders(fields.b.reminders ? subJsons[REMINDERS] : NULL, fields.b.events ? subJsons[EVENTS] : NULL);
		messageView->setMessage("New events and reminders. Press A to see reminder and B to see events");
		setView(messageView);
		blinkLED(NUM_BLINKS_MESSAGE);
		sendAction(RefreshScreen, true);
		nvs->set_str("jsons", cleanStr.c_str());
		nvs->commit();
	}

	if (fields.b.message) {
		std::string msgToDisplay = json_getValue(subJsons[MESSAGES]);
		messageView->setMessage(msgToDisplay);
		setView(messageView);
		blinkLED(NUM_BLINKS_MESSAGE);
		sendAction(RefreshScreen, true);
	}

}

void BadgerAgent::loadJSONFromNVS(void) {
	
	if (nvs->contains("jsons")) {

		size_t maxLen = BADGER_JSON_LEN;
		char jsonStr[maxLen];
		nvs->get_str("jsons", jsonStr, &maxLen);
		json_t const* subJsons[NUM_FIELDS];
		JsonFieldFlag fields = jsonFieldsPresent(jsonStr, subJsons);
		if (fields.b.events|| fields.b.reminders) {
			parseJSONEventsReminders(fields.b.reminders ? subJsons[REMINDERS] : NULL, fields.b.events ? subJsons[EVENTS] : NULL);
		}
		else {
			LogError(("Json saved in NVS does not contain reminders or events, deleting entry"));
			nvs->erase_key("jsons");
			nvs->commit();
		}
	}
	else {
		LogInfo(("No json in NVS, starting with a clean slate"));
	}

}

/***
* Parse a JSON string and add request to queue
* * @param str - JSON Strging
*/
JsonFieldFlag BadgerAgent::jsonFieldsPresent(char *str, json_t const* outJsons[]){
	
	JsonFieldFlag fields;
	json_t const* json = json_create( str, pJsonPool, BADGER_JSON_POOL);
	if ( !json ) {
		LogError(("Error json create."));
		return fields;
	}
	
	
	json_t const* remindersJson = json_getProperty( json, "reminders" );
	if ( !remindersJson|| JSON_ARRAY != json_getType( remindersJson) ) {
		LogDebug(("The reminders property is not found."));

	}
	else {
		fields.b.reminders = 1;
		outJsons[REMINDERS] = remindersJson;
	}

	json_t const* eventJson = json_getProperty( json, "calendar" );
	if ( !eventJson|| JSON_ARRAY != json_getType( eventJson) ) {
		LogDebug(("The calendar property is not found."));
	}
	else {
		fields.b.events = 1;
		outJsons[EVENTS] = eventJson;
	}

	json_t const* messageJson = json_getProperty( json, "message" );
	if ( !messageJson || JSON_TEXT != json_getType( messageJson ) ) {
		LogDebug(("The message property is not found."));
	}
	else {
		fields.b.message = 1;
		outJsons[MESSAGES] = messageJson;

	}
	return fields;
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
