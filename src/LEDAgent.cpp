/*
 * LEDAgent.cpp
 *
 * Manage LED and latched switch software behaviour
 *
 *  Created on: 23 Oct 2022
 *      Author: jondurrant
 */

#include "LEDAgent.h"
#include "MQTTTopicHelper.h"
#include "pico/cyw43_arch.h"

//Local enumerator of the actions to be queued
enum LEDAction {LEDOff, LEDOn, LEDToggle};

/***
 * Constructor
 * @param ledGP - GPIO Pad of LED to control
 * @param spstGP - GPIO Pad of SPST non latched switch
 * @param interface - MQTT Interface that state will be notified to
 */
LEDAgent::LEDAgent(uint8_t ledGP, uint8_t spstGP, MQTTInterface *interface) {
	xLedGP  = ledGP;
	xSpstGP = spstGP;
	pInterface = interface;

	//Initialise the LED and set it's initial state
//	gpio_init(xLedGP);
//	gpio_set_dir(xLedGP, GPIO_OUT);
//	gpio_put(xLedGP, 0);

	//Construct switch observer and listen
	pSwitchMgr = new SwitchMgr(xSpstGP, false);
	pSwitchMgr->setObserver(this);

	// Queue for actions commands for the class
	xCmdQ = xQueueCreate( LED_QUEUE_LEN, sizeof(LEDAction));
	if (xCmdQ == NULL){
		LogError(("Unable to create Queue\n"));
	}

	//Construct a message buffer
	xBuffer = xMessageBufferCreate(LED_BUFFER_LEN);
	if (xBuffer == NULL){
		LogError(("Buffer could not be allocated\n"));
	}

	//Construct the TOPIC for status messages
	if (pInterface != NULL){
		if (pTopicLedState == NULL){
			pTopicLedState = (char *)pvPortMalloc( MQTTTopicHelper::lenThingTopic(pInterface->getId(), MQTT_TOPIC_LED_STATE));
			if (pTopicLedState != NULL){
				MQTTTopicHelper::genThingTopic(pTopicLedState, pInterface->getId(), MQTT_TOPIC_LED_STATE);
			} else {
				LogError( ("Unable to allocate topic") );
			}
		}
	}

}

/***
 * Destructor
 */
LEDAgent::~LEDAgent() {
	if (pSwitchMgr != NULL){
		delete pSwitchMgr;
	}
	if (xCmdQ != NULL){
		vQueueDelete(xCmdQ);
	}
	if (pTopicLedState != NULL){
		vPortFree(pTopicLedState);
		pTopicLedState = NULL;
	}
	if (xBuffer != NULL){
		vMessageBufferDelete(xBuffer);
	}
}


/***
 * Handle a short press from the switch
 * @param gp - GPIO number of the switch
 */
void LEDAgent::handleShortPress(uint8_t gp){
	intToggle();
}

/***
 * Handle a short press from the switch
 * @param gp - GPIO number of the switch
 */
void LEDAgent::handleLongPress(uint8_t gp){
	intToggle();
}


/***
 * Set the states of the LED to - on
 * @param on - boolean if the LED should be on or off
 */
void LEDAgent::setOn(bool on){
	LEDAction action = LEDOff;

	if (on){
		action = LEDOn;
	}

	BaseType_t res = xQueueSendToBack(xCmdQ, (void *)&action, 0);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}


/***
 * Toggle the state of the LED. so On becomes Off, etc.
 */
void LEDAgent::toggle(){
	LEDAction action = LEDToggle;
	BaseType_t res = xQueueSendToBack(xCmdQ, (void *)&action, 0);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}

/***
 * Toggle LED state from within an intrupt
 */
void LEDAgent::intToggle(){
	LEDAction action = LEDToggle;
	BaseType_t res = xQueueSendToFrontFromISR(xCmdQ, (void *)&action, NULL);
	if (res != pdTRUE){
		LogWarn(("Queue is full\n"));
	}
}


/***
  * Main Run Task for agent
  */
void LEDAgent::run(){
	BaseType_t res;
	LEDAction action = LEDOff;
	char jsonStr[LED_JSON_LEN];
	size_t readLen;

	if (xCmdQ == NULL){
		return;
	}

	while (true) { // Loop forever
		readLen = xMessageBufferReceive(
						 xBuffer,
				         jsonStr,
						 LED_JSON_LEN,
						 0
				    );
		if (readLen > 0){
			jsonStr[readLen] = 0;
	        parseJSON(jsonStr);
		}

		res = xQueueReceive(xCmdQ, (void *)&action, 0);
		if (res == pdTRUE){
			switch(action){
				case LEDOff:{
					execLed(false);
					break;
				}
				case LEDOn:{
					execLed(true);
					break;
				}
				case LEDToggle:{
					execLed(!xState);
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
void LEDAgent::execLed(bool state){
	xState = state;
	//gpio_put(xLedGP, xState);
	cyw43_arch_gpio_put(xLedGP, xState);
	char payload[16];
	if (xState){
		sprintf(payload, "{\"on\"=True}");
	} else {
		sprintf(payload, "{\"on\"=False}");
	}
	if (pInterface != NULL){
		pInterface->pubToTopic(
			pTopicLedState,
			payload,
			strlen(payload),
			1,
			false
			);
	}
}



/***
 * Get the static depth required in words
 * @return - words
 */
 configSTACK_DEPTH_TYPE LEDAgent::getMaxStackSize(){
	 return 250;
 }


/***
* Parse a JSON string and add request to queue
* @param str - JSON Strging
*/
void LEDAgent::parseJSON(char *str){
	 json_t const* json = json_create( str, pJsonPool, LED_JSON_POOL);
	 if ( !json ) {
		 LogError(("Error json create."));
		 return ;
	 }
	 json_t const* on = json_getProperty( json, "on" );
	 if ( !on || JSON_BOOLEAN != json_getType( on ) ) {
		 LogError(("Error, the on property is not found."));
		 return ;
	 }
	 bool b = (int)json_getBoolean( on );

	 setOn(b);
}


/***
 * Add a JSON string action
 * @param jsonStr
 */
void LEDAgent::addJSON(const void  *jsonStr, size_t len){
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
