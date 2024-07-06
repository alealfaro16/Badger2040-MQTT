#ifndef EVENTVIEW_H 
#define EVENTVIEW_H 

#include "badger2040.hpp"
#include "View.h"
#include "EventReminder.h"
#include "logging_stack.h"

using namespace pimoroni;
using event_t = eventReminder_t;

constexpr float TITLE_TEXT_SIZE = 0.7f;
constexpr int MAX_EVENTS_DISPLAYED = 3; //With the text size this is the most we can show at one time

class EventView : public View {
	
public:
	EventView(Badger2040& badge, int& skipCount) : View(badge) , skipDisplayCount(skipCount){}
        void displayView(void) override;
        

        void addEvent(event_t newEvent) {
                eventVec.push_back(newEvent);
        }
        void clear(void) {
                eventVec.clear();
        }

        bool isEmpty(void) {
                return eventVec.empty();
        }

        void scrollUpdate(bool isUp) {
                eventStartIndex = isUp ? (eventStartIndex - 3) : (eventStartIndex + 3);
		        eventStartIndex = std::clamp(eventStartIndex, 0, static_cast<int>(eventVec.size() - 1));
                LogInfo(("New event start index is %d", eventStartIndex));
        }
        
        std::string msg;
        int& skipDisplayCount;
        std::vector<event_t> eventVec;
        int eventStartIndex = 0;
};

#endif
