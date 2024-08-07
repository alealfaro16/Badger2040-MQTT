/*
 * MQTTRouterBadger.h
 *
 *  Created on: 4 Oct 2022
 *      Author: jondurrant
 */

#ifndef _MQTTROUTERBADGER_H_
#define _MQTTROUTERBADGER_H_

#include "pico/stdlib.h"
#include "MQTTRouter.h"

#include "tiny-json.h"
#include "BadgerAgent.h"

#define MQTT_BADGER_REQ_TOPIC 	"Badger/req"

class MQTTRouterBadger : public MQTTRouter{
public:
	/***
	 * Constructor
	 */
	MQTTRouterBadger(BadgerAgent *agent);
	virtual ~MQTTRouterBadger();

	/***
	 * Use the interface to setup all the subscriptions
	 * @param interface
	 */
	virtual void subscribe(MQTTInterface *interface);

	/***
	 * Route the message the appropriate part of the application
	 * @param topic
	 * @param topicLen
	 * @param payload
	 * @param payloadLen
	 * @param interface
	 */
	virtual void route(const char *topic, size_t topicLen, const void * payload,
			size_t payloadLen, MQTTInterface *interface);



private:
	BadgerAgent *pAgent = NULL;
	char *pBadgerTopic = NULL;

};


#endif /*_MQTTROUTERBADGER_H_ */
