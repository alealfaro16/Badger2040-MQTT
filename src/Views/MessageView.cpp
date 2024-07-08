#include "MessageView.h"
#include "View.h"
#include "logging_stack.h"
#include "ViewUtil.h"


void MessageView::displayView(void) {
	skipDisplayCount = 2; //Skip clock display for 2 minutes

	badger.clearToWhite();

	std::vector<std::string> lines = breakStringForDisplaying(msg, TEXT_CHAR_LINE_MAX);
	int lineNum = 0;
	for (auto it = lines.begin(); it != lines.begin() + MAX_TEXT_LINES && it != lines.end(); it++){	
		badger.text(*it, TEXT_PADDING, (lineNum++)*TEXT_SPACING + TOP_MARGIN, TEXT_SIZE);
	}
	badger.update();

}
