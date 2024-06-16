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
#include "badger2040.hpp"
#include "logging_stack.h"
#include "tiny-json.h"
#include <string.h>

//Local enumerator of the actions to be queued

#define DISPLAY_WIDTH 340 //Needs tuning
#define DISPLAY_HEIGHT 128
#define TEXT_PADDING  4
constexpr float TEXT_SIZE = 0.5f;
#define TEXT_WIDTH  (DISPLAY_WIDTH - TEXT_PADDING - TEXT_PADDING)
/***
 * Constructor
 * @param ledGP - GPIO Pad of LED to control
 * @param spstGP - GPIO Pad of SPST non latched switch
 * @param interface - MQTT Interface that state will be notified to
 */
BadgerAgent::BadgerAgent(MQTTInterface *interface) {

	badger.init();
	xSpstGP = Badger2040::A;
	pInterface = interface;

	//Initialize variables to write text in display
	textSpacing = 34*TEXT_SIZE;
	lenPerChar = badger.measure_text("a", TEXT_SIZE);
	charPerLine = TEXT_WIDTH/lenPerChar;
	LogDebug(("len per char: %d and charPerLine %d", lenPerChar, charPerLine));
	badger.pen(15);
	badger.clear();
	badger.pen(0);
	badger.font("serif");
	badger.thickness(2);
	badger.text("Initiating Badger....", TEXT_PADDING, textSpacing/2 + TEXT_PADDING, TEXT_SIZE);
	badger.update();
	//Construct switch observer and listen
	pSwitchMgr = new SwitchMgr(xSpstGP, false);
	pSwitchMgr->setObserver(this);

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

}

/***
 * Destructor
 */
BadgerAgent::~BadgerAgent() {
	if (pSwitchMgr != NULL){
		delete pSwitchMgr;
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
	intToggle();
}

/***
 * Handle a short press from the switch
 * @param gp - GPIO number of the switch
 */
void BadgerAgent::handleLongPress(uint8_t gp){
	intToggle();
}


/***
 * Set the states of the LED to - on
 * @param on - boolean if the LED should be on or off
 */
void BadgerAgent::setOn(bool on){
	BadgerAction action = BadLEDOff;

	if (on){
		action = BadLEDOn;
	}

	BaseType_t res = xQueueSendToBack(xCmdQ, (void *)&action, 0);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}

void BadgerAgent::sendAction(BadgerAction action){
	
	BaseType_t res = xQueueSendToBack(xCmdQ, (void *)&action, 0);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}


/***
 * Toggle the state of the LED. so On becomes Off, etc.
 */
void BadgerAgent::toggle(){
	BadgerAction action = BadLEDToggle;
	BaseType_t res = xQueueSendToBack(xCmdQ, (void *)&action, 0);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}

/***
 * Toggle LED state from within an intrupt
 */
void BadgerAgent::intToggle(){
	BadgerAction action = BadLEDToggle;
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
	BadgerAction action = BadLEDOff;
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
				case BadLEDOff:{
					execLed(false);
					break;
				}
				case BadLEDOn:{
					execLed(true);
					break;
				}
				case BadLEDToggle:{
					execLed(!xState);
					break;
				}
				case WriteToScreen:{
					writeToDisplay(msgToDisplay);
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

	char payload[16];
	if (xState){
		sprintf(payload, "{\"on\"=True}");
	} else {
		sprintf(payload, "{\"on\"=False}");
	}
	
	if (pInterface != NULL){
		pInterface->pubToTopic(
			pTopicBadgerState,
			payload,
			strlen(payload),
			1,
			false
			);
	}
}

void BadgerAgent::writeToDisplay(std::string msg){
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
		badger.text(line, TEXT_PADDING, r*17 + 10, TEXT_SIZE);

	}
	badger.update();

}


/***
 * Get the static depth required in words
 * @return - words
 */
 configSTACK_DEPTH_TYPE BadgerAgent::getMaxStackSize(){
	 return 1024;
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
	json_t const* on = json_getProperty( json, "on" );
	if ( !on || JSON_BOOLEAN != json_getType( on ) ) {
		LogInfo(("The on property is not found."));
	}
	else {
		bool b = (int)json_getBoolean( on );
		setOn(b);
	}
	json_t const* message = json_getProperty( json, "message" );
	if ( !message || JSON_TEXT != json_getType( message ) ) {
		LogInfo(("The message property is not found."));
	}
	else {
		msgToDisplay = json_getValue(message);
		sendAction(WriteToScreen);
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
