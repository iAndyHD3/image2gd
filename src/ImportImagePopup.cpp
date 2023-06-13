#include "ImportImagePopup.hpp"
#include "stdio.h"

ImportImagePopup* ImportImagePopup::create()
{
	ImportImagePopup* ret = new ImportImagePopup();
	if(!ret || !ret->init()) {
		ret = nullptr;
	}
	return ret;
}

bool ImportImagePopup::init()
{
	if(!BrownAlertDelegate::init(350, 250, "GJ_square01.png", "image2gd")) {
		return false;
	}
	
	
	return true;
}

void ImportImagePopup::setup()
{
	
}

void ImportImagePopup::keyDown(cocos2d::enumKeyCodes key)
{
	printf("key: %d\n", static_cast<int>(key));
}