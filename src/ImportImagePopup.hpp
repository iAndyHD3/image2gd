#pragma once

	
#include <gd.h>
#include "BrownAlertDelegate.hpp"
#include "geometrize/bitmap/bitmap.h"
#include <atomic>

class ImportImagePopup : public BrownAlertDelegate {
public:
	
	geometrize::Bitmap _bitmap = geometrize::Bitmap(0, 0, geometrize::rgba{0, 0, 0, 0});
	std::atomic<bool> _bitmapLoaded = false;
	
	gd::CCTextInputNode* _totalShapesInput;
	gd::CCTextInputNode* _shapesPerStepInput;
	gd::CCTextInputNode* _scaleInput;
	gd::CCTextInputNode* _zLayerInput;
	
	cocos2d::CCLabelBMFont* _bottomLabel;
	
	gd::CCMenuItemSpriteExtra* _importBtn;
	
	struct Option
	{
		gd::CCTextInputNode** input = nullptr;
		const char* title = nullptr;
		const char* description = nullptr;
		const char* inputDefault = nullptr;
		cocos2d::CCPoint titleLabelPos;
		bool floatOption = false;
	};
	
	
public:
	static ImportImagePopup* create();
	static int calculateOptimalTotalShapes(int width, int height, bool optimizedForSpeed = true);
	static int calculateOptimalShapesPerStep(int width, int height, bool optimizedForSpeed = true);

	void setup() override;
	//void keyDown(cocos2d::enumKeyCodes) override;
	
	void importImageClipboard();
	void selectImageFileDialog();
	void importImageEditor();
	void addLabel(int i);
	void onHelp(cocos2d::CCObject*);
	void onImportImage(cocos2d::CCObject*);
	void onCalculateDefaultValues(cocos2d::CCObject*);
	void update(float dt);
	
	ImportImagePopup::Option getOption(int n);
};

