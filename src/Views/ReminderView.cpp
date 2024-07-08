#include "ReminderView.h"
#include "View.h"
#include "logging_stack.h"
#include <stdexcept>
#include "ViewUtil.h"

void ReminderView::displayView(void) {
	
	badger.pen(15);
	badger.clear();
	badger.pen(0);

	if (reminderVec.empty()) {
		badger.text("NO REMINDERS", TEXT_PADDING, TOP_MARGIN, TITLE_TEXT_SIZE);
		badger.update();
		return;
	}

	try {
		reminder_t& reminder = reminderVec.at(reminderIdx);

		skipDisplayCount = 2; //Skip clock display for 2 minutes
		//

		badger.text("Reminders", TEXT_PADDING, TOP_MARGIN, TITLE_TEXT_SIZE);

		std::string sideIndexStr = std::to_string(reminderIdx + 1) + "/" + std::to_string(reminderVec.size());
		badger.text(sideIndexStr, 4*DISPLAY_WIDTH/5 + 4*TEXT_PADDING , TOP_MARGIN, TITLE_TEXT_SIZE);
		
		int lineNum = 1;
		std::vector<std::string> lines = breakStringForDisplaying(reminder.title, TITLE_CHAR_LINE_MAX);
		for (auto& line : lines) {
			LogDebug(("line: %s", line.c_str()));
			badger.text(line, TEXT_PADDING, (lineNum++)*TITLE_TEXT_SPACING+ TOP_MARGIN, TITLE_TEXT_SIZE);
		}

		badger.text(reminder.time, TEXT_PADDING, 3*DISPLAY_HEIGHT/4 + 2*TEXT_PADDING, TITLE_TEXT_SIZE);
		badger.text(reminder.date, TEXT_PADDING, 7*DISPLAY_HEIGHT/8 + 2*TEXT_PADDING, TITLE_TEXT_SIZE);

		badger.update();

	}
	catch (const std::out_of_range& oor) {
		LogError(("Reminder idx is out of bounds of reminder vec!"));
	}

}
