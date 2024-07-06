
#ifndef REMINDERVIEW_H 
#define REMINDERVIEW_H

#include "EventReminder.h"
#include "badger2040.hpp"
#include "View.h"
#include "logging_stack.h"

using namespace pimoroni;

using reminder_t = eventReminder_t;

constexpr float REMINDER_TEXT_SIZE = 0.7f;
class ReminderView : public View {
	
public:
	/***
	 * Constructor
	 * @param interface - MQTT Interface that state will be notified to
	 */
	ReminderView(Badger2040& badge, int& skipCount) : View(badge), skipDisplayCount(skipCount){};
	void displayView(void) override;

    int& skipDisplayCount;


	int reminderIdx = 0;
	std::vector<reminder_t> reminderVec;

	void addReminder(reminder_t newReminder) {
		reminderVec.push_back(newReminder);
	}
	void clear(void) {
		reminderIdx = 0;
		reminderVec.clear();
	}

	bool isEmpty(void) {
		return reminderVec.empty();
	}
	int getIndex(void) {
		return reminderIdx;
	}
	bool setIndex(int idx) {
		idx = std::clamp(idx, 0, static_cast<int>(reminderVec.size() - 1));
		
		if (reminderIdx == idx) return false;

		LogInfo(("New reminder idx: %d", reminderIdx));
		reminderIdx = idx;

		return true;
	}
	
};

#endif
