#pragma once

#include "Request.h"
#include "tiny-json.h"

#define MAX_TAGS 80
class WeatherServiceRequest : public Request {

	public:
	WeatherServiceRequest() : Request(responseBuf, RESPONSE_BUF_LEN){}
	bool getWeather(std::string lat, std::string lon);
	bool getTempValues(float& temp, float& tempMin, float& tempMax);
	bool getDesc(char* desc);
	bool getLoc(char* loc);
	void getIcon();

private:
	//Weather Service parameters
	char xDesc[20];
	char xLoc[20];
	char xIcon[5];
	float xTemp = 0.0f;
	float xTempMax = 0.0f;
	float xTempMin = 0.0f;
	
	std::map<std::string, std::string> query;
	static constexpr int RESPONSE_BUF_LEN = 2048;
	char responseBuf[RESPONSE_BUF_LEN];
	char json[RESPONSE_BUF_LEN];
	json_t pool[MAX_TAGS];
	char * pBuffer;
	size_t xBufSize;
};
