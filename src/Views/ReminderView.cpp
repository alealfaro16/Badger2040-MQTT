#include "ReminderView.h"
#include "View.h"
#include "logging_stack.h"
#include <stdexcept>


void ReminderView::displayView(void) {
	
	badger.pen(15);
	badger.clear();
	badger.pen(0);

	if (reminderVec.empty()) {
		badger.text("NO REMINDERS", 10, 10, REMINDER_TEXT_SIZE);
		badger.update();
		return;
	}

	try {
		reminder_t& reminder = reminderVec.at(reminderIdx);

		skipDisplayCount = 2; //Skip clock display for 2 minutes
		//
		int lenChar = badger.measure_text("A", REMINDER_TEXT_SIZE);
		int cPerLine = TEXT_WIDTH/lenChar;
		int textSpacing = 34*REMINDER_TEXT_SIZE;


		int len = badger.measure_text(reminder.title, REMINDER_TEXT_SIZE);
		int rows = (len / TEXT_WIDTH) + 1;
		LogDebug(("Reminder title length is %d and it's display length is: %d", reminder.title.length(),len));
		LogDebug(("rows to print: %d",rows));
		badger.text("Reminder", 10, 10, REMINDER_TEXT_SIZE);
		for (int r = 0; r < rows; r++) {
			std::string line  = reminder.title.substr(r*cPerLine, cPerLine);
			LogDebug(("line: %s", line.c_str()));
			badger.text(line, TEXT_PADDING, r*textSpacing + 10 + textSpacing, REMINDER_TEXT_SIZE);
		}

		badger.text(reminder.time, TEXT_PADDING, 3*DISPLAY_HEIGHT/4, REMINDER_TEXT_SIZE);
		badger.text(reminder.date, TEXT_PADDING, 7*DISPLAY_HEIGHT/8, REMINDER_TEXT_SIZE);

		badger.update();

	}
	catch (const std::out_of_range& oor) {
		LogError(("Reminder idx is out of bounds of reminder vec!"));
	}

}
