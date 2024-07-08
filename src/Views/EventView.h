#ifndef EVENTVIEW_H 
#define EVENTVIEW_H 

#include "badger2040.hpp"
#include "View.h"
#include "EventReminder.h"
#include "logging_stack.h"

using namespace pimoroni;
using event_t = eventReminder_t;

constexpr int MAX_EVENTS_DISPLAYED = 3; //With the text size this is the most we can show at one time

class EventView : public View {
	
public:
	EventView(Badger2040& badge, int& skipCount) : View(badge) , skipDisplayCount(skipCount){}
        void displayView(void) override;
        

        int getEventNum(void) {
                return eventVec.size();
        }

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
                eventStartIndex = isUp ? (eventStartIndex - MAX_EVENTS_DISPLAYED) : (eventStartIndex + MAX_EVENTS_DISPLAYED);
		eventStartIndex = std::clamp(eventStartIndex, 0, static_cast<int>(eventVec.size() - 1));
                screenIdx = eventStartIndex/MAX_EVENTS_DISPLAYED + 1;
                LogInfo(("New event start index is %d", eventStartIndex));
        }
        
        int& skipDisplayCount;
        std::vector<event_t> eventVec;
        int eventStartIndex = 0;
        int screenIdx = 1;
};

#endif
