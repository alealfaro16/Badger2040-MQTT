#include "MessageView.h"
#include "View.h"
#include "logging_config.h"
#include "logging_stack.h"


void MessageView::displayView(void) {
	skipDisplayCount = 2; //Skip clock display for 2 minutes

	badger.pen(15);
	badger.clear();
	badger.pen(0);

	LogInfo(("msg received: %s",msg.c_str()));
	int len = badger.measure_text(msg, TEXT_SIZE);
	int rows = (len / TEXT_WIDTH) + 1;
	for (int r = 0; r < rows; r++) {
		std::string line  = msg.substr(r*CHAR_PER_LINE, CHAR_PER_LINE);
		badger.text(line, TEXT_PADDING, r*TEXT_SPACING+ 10, TEXT_SIZE);
	}
	badger.update();

}
