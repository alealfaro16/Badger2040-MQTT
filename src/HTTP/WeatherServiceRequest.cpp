#include "WeatherServiceRequest.h"
#include <spng.h>

bool WeatherServiceRequest::getWeather(std::string lat, std::string lon) {
	
	bool res;
	//Call OpenWeatherMap Service
	char url[] = "https://api.openweathermap.org/data/2.5/weather";

	query["appid"]=OPENWEATHERMAPKEY;
	query["lat"] = lat; 
	query["lon"] = lon;
	query["units"] = "imperial";

	res = get(url, &query);
	if ( res ){
		res = (getStatusCode() == 200);
	}
	if (res){
		int len = getPayloadLen();
		const char * payload = getPayload();
		printf("Parsing payload: %d\n", len); 

		memcpy(json, payload, len);
		json[len] = 0;
		json_t const* parent = json_create(
			json,
			pool,
		MAX_TAGS);
		if ( parent != NULL ){
			//Location
			json_t const* sys = json_getProperty( parent, "sys" );
			if (sys != NULL){
				json_t const* loc = json_getProperty( parent, "name" );
				if (loc != NULL){
					strcpy(xLoc, json_getValue(loc));
				}
			}

			//Temperatures
			json_t const* main = json_getProperty( parent, "main" );
			if (main != NULL){
				json_t const* temp = json_getProperty( main, "temp" );
				if (temp != NULL){
					if ( json_getType( temp ) == JSON_REAL ){
						xTemp = json_getReal(temp);
					}
				}
				json_t const* tempMin = json_getProperty( main, "temp_min" );
				if (tempMin != NULL){
					if ( json_getType( tempMin ) == JSON_REAL ){
						xTempMin = json_getReal(tempMin);
					}
				}
				json_t const* tempMax = json_getProperty( main, "temp_max" );
				if (tempMax != NULL){
					if ( json_getType( tempMax ) == JSON_REAL ){
						xTempMax = json_getReal(tempMax);
					}
				}
				printf("Temp: %f %f %f\n", xTemp, xTempMin, xTempMax);
			}

			//Icon
			json_t const* weather = json_getProperty( parent, "weather" );
			if (weather != NULL){
				json_t const* item = json_getChild( weather);
				if (item != NULL){
					json_t const* desc = json_getProperty( item, "description" );
					if (desc != NULL){
						strcpy(xDesc, json_getValue(desc));
					}
					json_t const* icon = json_getProperty( item, "icon" );
					if (icon != NULL){
						strcpy(xIcon, json_getValue(icon));
					}
				}
			}

		}
	} else {
		printf("WS failed %d\n", getStatusCode());
	}

	return res;
}

void WeatherServiceRequest::getIcon(){
	char iconURL[120];

	sprintf(iconURL,"https://openweathermap.org/img/w/%s.png", xIcon);
	Request req(pBuffer, xBufSize);
	bool res;

	res = req.get(iconURL);
	if ( res ){
		res = (req.getStatusCode() == 200);
	}
	if (res){
		printf("Icon Len: %d\n", req.getPayloadLen());

		size_t size;
		int length =  req.getPayloadLen();
		uint8_t * pPng = (uint8_t *) req.getPayload();
		struct spng_ihdr ihdr;
		struct spng_alloc spngAlloc;
		spngAlloc.malloc_fn = pvPortMalloc;
		//spngAlloc.realloc_fn = pvPortRealloc;
		//spngAlloc.calloc_fn = pvPortCalloc;
		spngAlloc.free_fn = vPortFree;

		spng_ctx *ctx = spng_ctx_new2(&spngAlloc, 0);
		spng_set_png_buffer(ctx, pPng, length);

		spng_decoded_image_size(ctx, SPNG_FMT_RGB8, &size);
		printf("PNG Out Buf size is %d\n", size);
		//out = (uint32_t *)malloc(size);
		uint8_t * out = (uint8_t *) pvPortMalloc(size);

		spng_get_ihdr(ctx, &ihdr);
		uint width = ihdr.width;
		uint height=ihdr.height;
		printf("PNG (%u, %u)\n", width, height);

		int res = spng_decode_image(ctx, out, size, SPNG_FMT_RGB8, 0);

		printf("Decod complete = %d  %s\n", res, spng_strerror(res));
	
		//Draw PNG
		//pTft->drawBitmapRGB8(0,0, width, height, out, true);
		vPortFree(out);
		spng_ctx_free(ctx);

	} else {
		printf("WS failed %d\n", req.getStatusCode());
	}
}

bool WeatherServiceRequest::getTempValues(float& temp, float& tempMin, float& tempMax) {
	
	if (xTemp == 0.0f || xTempMin == 0.0f || xTempMax == 0.0f){
		return false;
	}
	temp = xTemp;
	tempMin = xTempMin;
	tempMax = xTempMax;
	return true;
}

bool WeatherServiceRequest::getDesc(char* desc) {
	strcpy(desc, xDesc);
	return true;
}

bool WeatherServiceRequest::getLoc(char* loc) {
	strcpy(loc, xLoc);
	return true;
}

