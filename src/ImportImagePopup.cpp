#include "ImportImagePopup.hpp"
#include "stdio.h"
#include "PaginatedFLAlert.h"
#include "image2gd.h"
#include <fmt/format.h>
#include "nfd.h"

using namespace cocos2d;

ImportImagePopup* ImportImagePopup::create()
{
	ImportImagePopup* ret = new ImportImagePopup;
	if(!ret || !ret->init(430.0f, 270.0f, "GJ_square01.png", "image2gd")) {
		ret = nullptr;
	}
	return ret;
}

ImportImagePopup::Option ImportImagePopup::getOption(int i)
{
	auto winSize = CCDirector::sharedDirector()->getWinSize() / 2;
	float mx = winSize.width;
	float my = winSize.height;
	constexpr float sepY = 40.0f;

	ImportImagePopup::Option options;
	options.titleLabelPos = CCPoint(mx  - 150.0f, (my + 60.0f) - (sepY * i));

	switch(i)
	{
		case 0:
		{
			options.title = "Total Shapes:";
			options.description = 
			"This option determines the total number of shapes that will be generated in the final output (the object count)\n";
			
			options.inputDefault = "500";
			options.input = &_totalShapesInput;
			break;
		}
		case 1:
		{
			options.title = "Shapes per step:";
			options.description =
			"This option determines the number of shapes generated at each step of the process. "
			"A \"step\" is a single iteration of shape generation and selection. If there are 500 total shapes, there are 500 steps. Each step will generate the given amount of textures and choose the best scoring one per step.|"
			"Higher values result in more shapes being generated and evaluated at each step, which can lead to better quality but slower performance. "
			"Conversely, lower values generate fewer shapes at each step, resulting in faster performance but potentially sacrificing some quality.";
			options.inputDefault = "100";
			options.input = &_shapesPerStepInput;
			
			break;
		}
		case 2:
		{
			options.title = "Scale:";
			options.description = "The sale of the GD Objects. 1 is default and recommended. use a point as the decimal, eg 0.5";
			options.inputDefault = "1.0";
			options.input = &_scaleInput;
			options.floatOption = true;
			break;
		}
		case 3:
		{
			options.title = "Z Layer:";
			options.description = "GD Editor Z Layer where the objects will be created";
			options.inputDefault = "0";
			options.input = &_zLayerInput;
			break;
		}
	}
	return options;
}
void ImportImagePopup::setup()
{
	for(int i = 0; i < 4; i++)
	{
		addLabel(i);
	}
	auto winSizeMid = CCDirector::sharedDirector()->getWinSize() / 2;

	_bottomLabel = CCLabelBMFont::create("", "goldFont.fnt");
	_bottomLabel->setScale(0.6f);
	_bottomLabel->setAlignment(kCCTextAlignmentCenter);
	_bottomLabel->setPosition({winSizeMid.width, winSizeMid.height - 100 });
	m_pLayer->addChild(_bottomLabel);
	

	auto spr = gd::ButtonSprite::create("From Clipboard", 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 1);
	spr->setScale(0.8f);
	auto btn = gd::CCMenuItemSpriteExtra::create(spr, this, menu_selector(ImportImagePopup::onImportImage));
	btn->setTag(1);
	btn->setPosition(m_pButtonMenu->convertToNodeSpace({winSizeMid.width + 110.0f, winSizeMid.height + 63.0f}));
	m_pButtonMenu->addChild(btn);
	
	spr = gd::ButtonSprite::create("Open file", 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 1);
	spr->setScale(0.8f);
	btn = gd::CCMenuItemSpriteExtra::create(spr, this, menu_selector(ImportImagePopup::onImportImage));
	btn->setTag(2);
	btn->setPosition(m_pButtonMenu->convertToNodeSpace({winSizeMid.width + 85.0f, winSizeMid.height + 25.0f}));
	m_pButtonMenu->addChild(btn);
	
	spr = gd::ButtonSprite::create("Import", 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 1);
	spr ->setScale(0.8f);
	_importBtn = gd::CCMenuItemSpriteExtra::create(spr, this, menu_selector(ImportImagePopup::onImportImage));
	_importBtn->setTag(3);
	_importBtn->setVisible(false);
	_importBtn->setPosition(m_pButtonMenu->convertToNodeSpace({winSizeMid.width + 85.0f, winSizeMid.height - 30.0f}));
	m_pButtonMenu->addChild(_importBtn);
	
	
}

void ImportImagePopup::onImportImage(CCObject* sender)
{
	int tag = reinterpret_cast<CCNode*>(sender)->getTag();
	switch(tag)
	{
		case 1: return importImageClipboard();
		case 2: return selectImageFileDialog();
		case 3: return importImageEditor();
	}
}

void ImportImagePopup::importImageEditor()
{
	if(!gd::LevelEditorLayer::get()) return;
	
	auto alert = [](const char* title, const char* desc) { gd::FLAlertLayer::create(nullptr, title, "OK", nullptr, 200, desc)->show(); };
	
	int totalShapes = _stoi(_totalShapesInput->getString());
	if(totalShapes < 1) return alert("Total Shapes", "Total shapes must be greater than 0");
	TOTAL_SHAPES = totalShapes;
	
	int shapesPerStep = _stoi(_shapesPerStepInput->getString());
	if(shapesPerStep < 1) return alert("Shapes Per Step", "Shapes per step must be greater than 0");
	
	float scale = _stof(_scaleInput->getString());
	if(scale == 0.0f) return alert("Scale", "Scale can not be 0. for default set 1");
	DRAW_SCALE = scale;
	
	const char* zlayertxet = _zLayerInput->getString();
	int zLayer = zlayertxet ? _stoi(zlayertxet) : 0;
	if(zLayer < 0) return alert("Z Layer", "Z Layer must be a positive value");
	Z_LAYER = zLayer;

	DRAWN_SHAPES = 0;
	PROCESSING_IMAGE = true;
	BTN_SPR->setString("Stop current");
	enableUpdateHook();
	std::thread(addImage, _bitmap, totalShapes, shapesPerStep).detach();
	onClose(nullptr);
}

void ImportImagePopup::selectImageFileDialog()
{
	nfdchar_t* outPath = nullptr;
	nfdresult_t result = NFD_OpenDialog("png,jpg", nullptr, &outPath);
	if(result == NFD_OKAY)
	{
		_bitmap = readImage(outPath);
		if(isValidBitmap(_bitmap))
		{
			_importBtn->setVisible(true);
			std::string str = fmt::format("Loaded Image from file\nWidth: {} | Height: {}", _bitmap.getWidth(), _bitmap.getHeight());
			_bottomLabel->setString(str.c_str());
		}
		else
		{
			_bottomLabel->setString("Something went wrong");
			_importBtn->setVisible(false);
		}
		free(outPath);
	}
}
	
void ImportImagePopup::addLabel(int i)
{
	auto options = ImportImagePopup::getOption(i);
	constexpr const char* _font = "chatFont.fnt";
	constexpr const char* _inputFont = "bigFont.fnt";
	
	auto totalShapesLabel = CCLabelBMFont::create(options.title, _font);
	totalShapesLabel->setScale(.8f);
	totalShapesLabel->setPosition(options.titleLabelPos);
	m_pLayer->addChild(totalShapesLabel);
	

	auto input = gd::CCTextInputNode::create(
		"",
		this,
		_inputFont,
		50.0f,
		50.0f
	);
	input->setString(options.inputDefault);
	input->setScale(.8f);
	input->setPosition({options.titleLabelPos.x + 95.0f, options.titleLabelPos.y});
	input->setAllowedChars(options.floatOption ? ".0123456789" : "0123456789");
	input->setMaxLabelLength(5);
	//_totalShapesInput->setMaxLabelWidth(100.0f);
	//_totalShapesInput->setMaxLabelScale(1.0f);
	m_pLayer->addChild(input, 5);
	*options.input = input;
	
	auto inputBG = cocos2d::extension::CCScale9Sprite::create("square02_small.png", {0.0f, 0.0f, 40.0f, 40.0f});
	inputBG->setContentSize({70.0f, 30.0f});
	inputBG->setOpacity(100);
	inputBG->setPosition({input->getPositionX() + 3.0f, input->getPositionY() + 4.0f});
	m_pLayer->addChild(inputBG);
	
	auto helpSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
	auto helpBtn = gd::CCMenuItemSpriteExtra::create(helpSpr, this, menu_selector(ImportImagePopup::onHelp));
	helpBtn->setTag(i);
	helpBtn->setPosition(m_pButtonMenu->convertToNodeSpace({input->getPositionX() + 60.0f, input->getPositionY() + 4.0f}));
	m_pButtonMenu->addChild(helpBtn);
}


std::vector<std::string> splitByDelim(const std::string& str, char delim)
{
	std::vector<std::string> tokens;
	size_t pos = 0;
	size_t len = str.length();
	tokens.reserve(len / 2);  // allocate memory for expected number of tokens
	
	while (pos < len)
	{
		size_t end = str.find_first_of(delim, pos);
		if (end == std::string::npos)
		{
			tokens.emplace_back(str.substr(pos));
			break;
		}
		tokens.emplace_back(str.substr(pos, end - pos));
		pos = end + 1;
	}

	return tokens;
}


void ImportImagePopup::onHelp(CCObject* sender)
{
	ImportImagePopup::Option option = ImportImagePopup::getOption(sender->getTag());
	bool split = false;
	for(int i = 0; option.description[i]; i++)
	{
		if(option.description[i] == '|') {
			split = true;
			break;
		}
	}
	
	if(split)
	{
		PaginatedFLAlert::create(option.title, splitByDelim(option.description, '|'))->show();
	}
	else
	{
		gd::FLAlertLayer::create(nullptr, option.title, "OK", nullptr, 300.0f, option.description)->show();
	}
}

void ImportImagePopup::update(float dt)
{
	if(_bitmapLoaded)
	{
		_bitmapLoaded = false;
		if(!isValidBitmap(_bitmap))
		{
			_bottomLabel->setString("Something went wrong reading image from clipboard");
		}
		else
		{
			_importBtn->setVisible(true);
			std::string str = fmt::format("Loaded Image from clipboard\nWidth: {} | Height: {}", _bitmap.getWidth(), _bitmap.getHeight());
			_bottomLabel->setString(str.c_str());
		}
		unscheduleUpdate();
	}
}

void ImportImagePopup::importImageClipboard()
{
	//_importBtn->setVisible(false);
	_bottomLabel->setString("Preloading image\n(Use select file for instant load)");
	_bitmapLoaded = false;
	std::thread t([this]()
	{
		_bitmap = readImageFromClipboard();
		_bitmapLoaded = true;
	});
	scheduleUpdate();
	t.detach();
}


/*
void ImportImagePopup::keyDown(cocos2d::enumKeyCodes key)
{
	printf("key: %d\n", static_cast<int>(key));

	switch(key)
	{
		case enumKeyCodes::KEY_V: return importImageClipboard();
	}
	BrownAlertDelegate::keyDown(key);
}
*/