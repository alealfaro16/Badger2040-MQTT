/***
 * main.cpp - MQTT Connection
 * Jon Durrant
 * 4-Oct-2022
 *
 *
 */

#include "MusicAgent.h"
#include "WeatherServiceRequest.h"
#include "hardware/rtc.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <cstddef>
#include <stdio.h>

#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>


#include "WifiHelper.h"
#include "MQTTAgent.h"
#include "MQTTAgentObserver.h"
#include "BadgerAgent.h"
#include "MQTTRouterBadger.h"
#include "NVSOnboard.h"
#include "Request.h"


//Check these definitions where added from the makefile
#ifndef WIFI_SSID
#error "WIFI_SSID not defined"
#endif
#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD not defined"
#endif
#ifndef MQTT_CLIENT
#error "MQTT_CLIENT not defined"
#endif
#ifndef MQTT_USER
#error "MQTT_PASSWD not defined"
#endif
#ifndef MQTT_PASSWD
#error "MQTT_PASSWD not defined"
#endif
#ifndef MQTT_HOST
#error "MQTT_HOST not defined"
#endif
#ifndef MQTT_PORT
#error "MQTT_PORT not defined"
#endif


#define TASK_PRIORITY			( tskIDLE_PRIORITY + 1UL )

void runTimeStats(   ){
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	unsigned long ulTotalRunTime;


   /* Take a snapshot of the number of tasks in case it changes while this
   function is executing. */
   uxArraySize = uxTaskGetNumberOfTasks();
   printf("Number of tasks %d\n", uxArraySize);

   /* Allocate a TaskStatus_t structure for each task.  An array could be
   allocated statically at compile time. */
   pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

   if( pxTaskStatusArray != NULL ){
      /* Generate raw status information about each task. */
      uxArraySize = uxTaskGetSystemState( pxTaskStatusArray,
                                 uxArraySize,
                                 &ulTotalRunTime );



	 /* For each populated position in the pxTaskStatusArray array,
	 format the raw data as human readable ASCII data. */
	 for( x = 0; x < uxArraySize; x++ )
	 {
		 printf("Task: %d \t cPri:%d \t bPri:%d \t hw:%d \t%s\n",
				pxTaskStatusArray[ x ].xTaskNumber ,
				pxTaskStatusArray[ x ].uxCurrentPriority ,
				pxTaskStatusArray[ x ].uxBasePriority ,
				pxTaskStatusArray[ x ].usStackHighWaterMark ,
				pxTaskStatusArray[ x ].pcTaskName
				);
	 }


      /* The array is no longer needed, free the memory it consumes. */
      vPortFree( pxTaskStatusArray );
   } else {
	   printf("Failed to allocate space for stats\n");
   }

   HeapStats_t heapStats;
   vPortGetHeapStats(&heapStats);
   printf("HEAP avl: %d, blocks %d, alloc: %d, free: %d\n",
		   heapStats.xAvailableHeapSpaceInBytes,
		   heapStats.xNumberOfFreeBlocks,
		   heapStats.xNumberOfSuccessfulAllocations,
		   heapStats.xNumberOfSuccessfulFrees
		   );

}


void sntpInit(void) {

	//Setup SNTP to get time
	WifiHelper::sntpAddServer("0.uk.pool.ntp.org");
	WifiHelper::sntpAddServer("1.uk.pool.ntp.org");
	WifiHelper::sntpAddServer("2.uk.pool.ntp.org");
	WifiHelper::sntpAddServer("3.uk.pool.ntp.org");
	WifiHelper::sntpSetTimezone(-7); //California UTC offset
	WifiHelper::sntpStartSync();
}

const char mqttTarget[] = MQTT_HOST;
const int mqttPort = MQTT_PORT;
const char mqttClient[] = MQTT_CLIENT;
const char mqttUser[] = MQTT_USER;
const char mqttPwd[] = MQTT_PASSWD;
	
void MQTTInit(MQTTAgent& agent, MQTTAgentObserver& obs) {

	// Setup for MQTT Connection
	agent.setObserver(&obs);
	agent.credentials(mqttUser, mqttPwd, mqttClient);

	printf("Connecting to: %s(%d)\n", mqttTarget, mqttPort);
	printf("Client id: %.4s...\n", agent.getId());
	printf("User id: %.4s...\n", mqttUser);

	agent.mqttConnect(mqttTarget, mqttPort, true);
	agent.start(TASK_PRIORITY);

}

void checkWifi(void) {

	if (!WifiHelper::isJoined()){
		printf("AP Link is down\n");

		if (WifiHelper::join(WIFI_SSID, WIFI_PASSWORD)){
			printf("Connect to Wifi\n");
		} else {
			printf("Failed to connect to Wifi \n");
		}
	}
}


void getWifiCredentialNVS(char* ssid, char* password, size_t maxSize) {
	
	NVSOnboard* nvs = NVSOnboard::getInstance();
	nvs->printNVS();
	//Get SSID
	if (nvs->contains("SSID")){
		 size_t ssidSize = maxSize;
		 nvs->get_str("SSID", ssid, &ssidSize); 
		 printf("%s = %s\n", "SSID", ssid);
	} else {
		printf("%s Key not present\n", "SSID");
		nvs->set_str("SSID", WIFI_SSID);
		nvs->commit();
		strncpy(ssid,WIFI_SSID, maxSize);
	}
	
	//Get password
	if (nvs->contains("PASSWORD")){
		size_t passSize = maxSize;
		nvs->get_str("PASSWORD", password, &passSize); 
		printf("%s = %s\n", "PASSWORD", password);
	} else {
		printf("%s Key not present\n", "PASSWORD");
		nvs->set_str("PASSWORD", WIFI_PASSWORD);
		nvs->commit();
		strncpy(password, WIFI_PASSWORD, maxSize);
	}
}

void debugCB(const int logLevel, const char *const logMessage){
	printf("WOLFSSL DEBUG(%d): %s\n", logLevel, logMessage);
}

void wifiInit(void) {
	
	wolfSSL_Init();
	wolfSSL_SetLoggingCb( debugCB);
	//wolfSSL_Debugging_ON();

	if (WifiHelper::init()){
		printf("Wifi Controller Initialised\n");
	} else {
		printf("Failed to initialise controller\n");
		return;
	}
	
	//Check NVS for Wi-Fi credentials
	size_t sizeMaxVal = 30;
	char ssidVal[sizeMaxVal];
	char pswdVal[sizeMaxVal];
	getWifiCredentialNVS(ssidVal, pswdVal, sizeMaxVal); 


	printf("Connecting to WiFi... %s\n", ssidVal);

	if (WifiHelper::join(ssidVal, pswdVal)){
		printf("Connect to Wifi\n");
	} else {
		printf("Failed to connect to Wifi \n");
	}


	//Print MAC Address
	char macStr[20];
	WifiHelper::getMACAddressStr(macStr);
	printf("MAC ADDRESS: %s\n", macStr);

	//Print IP Address
	char ipStr[20];
	WifiHelper::getIPAddressStr(ipStr);
	printf("IP ADDRESS: %s\n", ipStr);

}


void main_task(void *params){

	printf("Main task started\n");
#if WIFI_ENABLED
	wifiInit();
	sntpInit();
	
	MQTTAgent mqttAgent;
	MQTTAgentObserver mqttObs;
	MQTTInit(mqttAgent, mqttObs);

	//Create Badger Agent and router
	BadgerAgent badAgent(&mqttAgent);
	badAgent.start("BadAgent", TASK_PRIORITY);
	MQTTRouterBadger badRouter(&badAgent);
	mqttAgent.setRouter(&badRouter);

#if BLE_ENABLED	
	BleServer_Init();
	BleServer_SetupBLE();
#endif
#elif  BLE_ENABLED
	BleServer_Init();
#else

#error "BLE nor WiFi is enabled"
#endif
	//Bind to CORE 1
	UBaseType_t coreMask = 0x2;
	vTaskCoreAffinitySet( mqttAgent.getTask(), coreMask );

	//Bind to CORE 0
	coreMask = 0x1;
	vTaskCoreAffinitySet( badAgent.getTask(), coreMask );

    while(true) {

    	//runTimeStats();

        vTaskDelay(3000);
#if WIFI_ENABLED
	checkWifi();
#endif

    }

}



#define MAIN_THREAD_STACK_SIZE	(1024*3) 

void vLaunch( void) {
    TaskHandle_t task;

    xTaskCreate(main_task, "MainThread", MAIN_THREAD_STACK_SIZE, NULL, TASK_PRIORITY, &task);

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}


int main( void )
{
    timer_hw->dbgpause = 0;
    stdio_init_all();
    sleep_ms(2000);
    printf("GO\n");

    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
    rtos_name = "FreeRTOS";
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();

    return 0;
}
