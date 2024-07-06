
#ifndef VIEW_H 
#define VIEW_H

#include "badger2040.hpp"
#include "logging_config.h"
#include <stdio.h>

const int DISPLAY_WIDTH = 340; //Needs tuning
const int DISPLAY_HEIGHT = 128;
constexpr float TEXT_SIZE = 0.5f;
constexpr float TEXT_SPACING = 34*TEXT_SIZE;
constexpr float CHAR_PER_LINE = 30; //Needs tuning

#define TEXT_WIDTH  (DISPLAY_WIDTH - TEXT_PADDING - TEXT_PADDING)
#define TEXT_PADDING  4


using namespace pimoroni;

class View {
	
public:
	Badger2040& badger;

	View(Badger2040& badge) : badger(badge) {}
	virtual void displayView(void) {
                badger.pen(15);
                badger.clear();
                badger.pen(0);
                badger.font("serif");
                badger.thickness(2);
                badger.text("Initiating Badger....", TEXT_PADDING, TEXT_SPACING + TEXT_PADDING, TEXT_SIZE);
                badger.update();
        }

        virtual ~View() {}
};

#endif
