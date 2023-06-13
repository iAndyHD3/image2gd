#pragma once

	
#include <gd.h>
#include "BrownAlertDelegate.hpp"
	
class ImportImagePopup : public BrownAlertDelegate {
public:
	
	static ImportImagePopup* create();

	bool init();
	void setup() override;
	void keyDown(cocos2d::enumKeyCodes) override;
};

