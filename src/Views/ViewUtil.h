#pragma once

#include <string>
#include <sstream>
#include <vector>
#include "hardware/rtc.h"
#include <time.h>
#include "WifiHelper.h"

using namespace std;
inline vector<string> breakStringForDisplaying(string str, int charPerLine) {

        vector<string> result(1);
        std::istringstream stm(str) ; // use a string stream to split on white-space
        std::string word ;
        while( stm >> word ) // for each word in the string
        {
                // if this word will fit into the current line, append the word to the current line
                if( ( result.back().size() + word.size() ) <= charPerLine ) {
                        result.back() += word + ' ' ;
                }
                else
        {
                        result.back().pop_back() ; // remove the trailing space at the end of the current line
                        result.push_back( word + ' ' ) ; // and place this new word on the next line
                }
        }

        result.back().pop_back() ; // remove the trailing space at the end of the last line
        return result ;

}


inline time_t get_seconds_from_datetime_t(datetime_t t)
{
    struct tm timeinfo;

    // Set the desired date and time
    timeinfo.tm_year = t.year - 1900; // Year - 1900
    timeinfo.tm_mon = t.month - 1; // Month (0-11)
    timeinfo.tm_mday = t.day; // Day of the month (1-31)
    timeinfo.tm_hour = t.hour; // Hour (0-23)
    timeinfo.tm_min = t.min; // Minute (0-59)
    timeinfo.tm_sec = t.sec; // Seconds (0-60)
	

    time_t sec = mktime(&timeinfo);
	return sec - (60 * WifiHelper::sntpTimezoneMinutesOffset);
}

