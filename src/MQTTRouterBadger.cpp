/*
 * MQTTRouterBadger.cpp
 *
 *  Created on: 4 Oct 2022
 *      Author: jondurrant
 */

#include "MQTTRouterBadger.h"
#include "MQTTTopicHelper.h"

#define BADGER_TOPIC  "Badger"
#define PAYLOAD_ON "on"

MQTTRouterBadger::MQTTRouterBadger(BadgerAgent *agent) {
	pAgent = agent;
}

MQTTRouterBadger::~MQTTRouterBadger() {
	// Nop
}



/***
 * Use the interface to setup all the subscriptions
 * @param interface
 */
void MQTTRouterBadger::subscribe(MQTTInterface *interface){
	//Init topic if needed
	if (pBadgerTopic == NULL){
		const char *id = interface->getId();
		pBadgerTopic = (char *)pvPortMalloc(
			MQTTTopicHelper::lenThingTopic(id, MQTT_BADGER_REQ_TOPIC)
			);
		if (pBadgerTopic != NULL){
			MQTTTopicHelper::genThingTopic(pBadgerTopic, id, MQTT_BADGER_REQ_TOPIC);
		} else {
			LogError( ("Unable to allocate topic") );
		}
	}
	if (pBadgerTopic != NULL){
		LogInfo(("Subscribing to topic: %s",pBadgerTopic)); 
		interface->subToTopic(pBadgerTopic, 1);
	}
}

/***
 * Route the message the appropriate part of the application
 * @param topic
 * @param topicLen
 * @param payload
 * @param payloadLen
 * @param interface
 */
void MQTTRouterBadger::route(const char *topic,
		size_t topicLen,
		const void * payload,
		size_t payloadLen,
		MQTTInterface *interface){

	if (pAgent != NULL){
		pAgent->addJSON(payload, payloadLen);
	}

}
