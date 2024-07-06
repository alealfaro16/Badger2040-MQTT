
#ifndef MESSAGEVIEW_H 
#define MESSAGEVIEW_H

#include "badger2040.hpp"
#include "View.h"

using namespace pimoroni;

class MessageView : public View {
	
public:
	MessageView(Badger2040& badge, int& skipCount) : View(badge) , msg("Hello world!"), skipDisplayCount(skipCount){}
        void setMessage(std::string msgToDisplay) { msg = msgToDisplay; };
        void displayView(void) override;
        
        std::string msg;
        int& skipDisplayCount;
};

#endif
