#include "EventView.h"
#include "View.h"
#include "hardware/rtc.h"

void EventView::displayView(void) {

	badger.pen(15);
	badger.clear();
	badger.pen(0);

	if (eventVec.empty()) {
		badger.text("NO EVENTS", 10, 10, TITLE_TEXT_SIZE);
		badger.update();
		return;
	}

	skipDisplayCount = 2; //Skip clock display for 2 minutes
	datetime_t d;
	rtc_get_datetime(&d);
	std::string titleText = "Events (" + std::to_string(d.month) + "/" + std::to_string(d.day) + ")";
	badger.text(titleText, 10, 10, TITLE_TEXT_SIZE);
	int titleTextSpacing = 34*TITLE_TEXT_SIZE;
	int yTopMargin = 10 + titleTextSpacing;
	int row = 0;
	for (auto it = eventVec.begin() + eventStartIndex; it != eventVec.begin() + eventStartIndex + MAX_EVENTS_DISPLAYED && it != eventVec.end(); it++) {
		auto event = *it;
		badger.text(event.time, DISPLAY_WIDTH/3, (row++)*TEXT_SPACING + yTopMargin ,TEXT_SIZE);
		badger.text(event.title, TEXT_PADDING, (row++)*TEXT_SPACING + yTopMargin, TEXT_SIZE);

		if (row*TEXT_SPACING> DISPLAY_HEIGHT) break;
	}
	badger.update();
}
