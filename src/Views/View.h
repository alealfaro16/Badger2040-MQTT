
#ifndef VIEW_H 
#define VIEW_H

#include "badger2040.hpp"
#include "logging_config.h"
#include <stdio.h>

//Display constants
const int DISPLAY_WIDTH = 296;
const int DISPLAY_HEIGHT = 128;

//Text constants
constexpr float TEXT_SIZE = 0.5f;
constexpr float TITLE_TEXT_SIZE = 0.7f;
constexpr float TEXT_SPACING = 34*TEXT_SIZE;
constexpr float TITLE_TEXT_SPACING = 34*TITLE_TEXT_SIZE;
constexpr int DEFAULT_THICKNESS = 2;
constexpr int TEXT_CHAR_LINE_MAX = 32;
constexpr int MAX_TEXT_LINES = 7;
constexpr int TITLE_CHAR_LINE_MAX = 20;

//Display position constants
constexpr int TOP_MARGIN = 10;
#define TEXT_WIDTH  (DISPLAY_WIDTH - TEXT_PADDING - TEXT_PADDING)
#define TEXT_PADDING  4


using namespace pimoroni;

class View {
	
public:
	Badger2040& badger;

	View(Badger2040& badge) : badger(badge) {}
	virtual void displayView(void) {
                badger.clearToWhite();
                badger.font("serif");
                badger.thickness(DEFAULT_THICKNESS);
                badger.text("Initiating Badger....", TEXT_PADDING, TEXT_SPACING + TEXT_PADDING, TEXT_SIZE);
                badger.update();
        }

        virtual ~View() {}
};

#endif
