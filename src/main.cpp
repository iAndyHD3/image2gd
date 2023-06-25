

#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>
#include <matdash/console.hpp>

#include <fmt/format.h>
#include <gd.h>

#include "image2gd.h"
#include "ImportImagePopup.hpp"

#include "geometrize/shape/circle.h"
#include "geometrize/bitmap/rgba.h"
#include "geometrize/shaperesult.h"


#define MEMBERBYOFFSET(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)
#define MBO MEMBERBYOFFSET

using namespace gd;
using namespace cocos2d;

//globals all in CAPS. image2gd.h

void updateLabel();

void addCircle(LevelEditorLayer* self, const geometrize::ShapeResult& result)
{
	if(!PROCESSING_IMAGE)
		return;
	
	geometrize::Circle* circle = dynamic_cast<geometrize::Circle*>(result.shape.get());
	
	geometrize::rgba color = result.color;
	
	float h,s,v;
	RGBtoHSV(color.r, color.g, color.b, h, s, v);

	std::string hsv_string = fmt::format("{:.2}a{:.2}a{:.2}a1a1", h, s, v);
	
	const float x = circle->m_x * DRAW_SCALE;
	const float y = circle->m_y * DRAW_SCALE;
	const float scale = circle->m_r * DRAW_SCALE / 4;
	
	//https://wyliemaster.github.io/gddocs/#/resources/client/level-components/level-object?id=level-object
	std::string str = fmt::format(
		"1,{},2,{},3,{},32,{},41,1,42,1,43,{},44,{},25,{},21,{},20,{}",
		CIRCLE_ID, x, y, scale, hsv_string, hsv_string, DRAWN_SHAPES.load(), 1010, Z_LAYER
	);
	//fmt::println("{}", str);
	self->addObjectFromString(str);
	DRAWN_SHAPES++;
	//fmt::print("added shape | x: {}, y: {} | scale: {} | {}\n", x, y, DRAW_SCALE, DRAWN_SHAPES);
	if(DRAWN_SHAPES == TOTAL_SHAPES)
	{
		PROCESSING_IMAGE = false;
	}
	
	updateLabel();
}


void updateLabel()
{
	gd::EditorUI* ui = gd::EditorUI::get();
	if(!ui) return;
	
	cocos2d::CCLabelBMFont* label = reinterpret_cast<cocos2d::CCLabelBMFont*>(ui->getChildByTag(33));
	if(!label)
	{
		auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize() / 2;
		label = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
		label->setPosition({winSize.width, winSize.height - 50.0f});
		label->setTag(33);
		ui->addChild(label);
	}
	int drawn_shapes = DRAWN_SHAPES.load();
	if(label && drawn_shapes > 0 && drawn_shapes < TOTAL_SHAPES)
	{
		float percentage = ((static_cast<float>(drawn_shapes) / static_cast<float>(TOTAL_SHAPES)) * 100);
		std::string labelstr = fmt::format("{}/{} | {:.2}%", drawn_shapes, TOTAL_SHAPES, percentage);
		label->setString(labelstr.c_str());
		label->setVisible(true);
	}
	else
	{
		label->setVisible(false);
	}
}

void hideLabel()
{
	gd::EditorUI* ui = gd::EditorUI::get();
	if(!ui) return;
	
	cocos2d::CCLabelBMFont* label = reinterpret_cast<cocos2d::CCLabelBMFont*>(ui->getChildByTag(33));
	if(!label)
	{
		auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize() / 2;
		label = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
		label->setPosition({winSize.width, winSize.height - 50.0f});
		label->setTag(33);
		ui->addChild(label);
	}
	if(label)
	label->setVisible(false);
}

void addShape(LevelEditorLayer* self, const geometrize::ShapeResult& result)
{
	switch(result.shape->getType())
	{
		case geometrize::ShapeTypes::CIRCLE: return addCircle(self, result);
	}
}

void (__thiscall* LevelEditorLayer_updateO)(void* self, float dt);
void __fastcall LevelEditorLayer_updateH(LevelEditorLayer* self, void* edx, float dt)
{
	if(PROCESSING_IMAGE) //technically not needed because hook gets enabled/disabled but left for security
	{
		SHAPE_LOCK.lock();
		if(SHAPE_DATA.size() != 0)
		{
			for(geometrize::ShapeResult result : SHAPE_DATA)
			{
				addShape(self, result);
			}
			SHAPE_DATA.clear();
			
			if(DRAWN_SHAPES >= TOTAL_SHAPES)
			{
				PROCESSING_IMAGE = false;
			}
		}
		SHAPE_LOCK.unlock();
	}
	
	LevelEditorLayer_updateO(self, dt);
}

struct Callback
{
	void onImportImage(CCObject* sender)
	{
		if(!PROCESSING_IMAGE)
		{
			ImportImagePopup::create()->show();
		}
		else
		{
			PROCESSING_IMAGE = false;
			hideLabel();
			auto btn = reinterpret_cast<gd::ButtonSprite*>(reinterpret_cast<CCNode*>(sender)->getChildren()->objectAtIndex(0));
			btn->setString("Import Image");
			disableUpdateHook();
		}
	}
};

bool (__thiscall* LevelSettingsLayer_initO)(CCLayer* self, void* settings, void* editor);
bool __fastcall LevelSettingsLayer_initH(CCLayer* self, void* edx, void* settings, void* editor)
{
	if(!LevelSettingsLayer_initO(self, settings, editor)) return false;
	if(!self) return true;
	
	bool startPosSettings = MBO(bool, settings, 272);
	if(startPosSettings) return true;
	//fmt::println("test");
	
	auto winSize = CCDirector::sharedDirector()->getWinSize() / 2;
	auto layer = (CCLayer*)self->getChildren()->objectAtIndex(0);
	if(!layer) return true;
	
	auto menu = (CCMenu*)layer->getChildren()->objectAtIndex(1);
	if(!menu) return true;
	
	const char* text = PROCESSING_IMAGE ? "Stop current" : "Import Image";
	BTN_SPR = ButtonSprite::create(text, 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 1);
	BTN_SPR->setScale(0.65f);
	BTN_SPR->setUserObject(self);
	auto btn = CCMenuItemSpriteExtra::create(BTN_SPR, self, menu_selector(Callback::onImportImage));
	btn->setPosition(menu->convertToNodeSpace({winSize.width - 100.0f, winSize.height - 130.0f}));
	menu->addChild(btn);
	return true;
	
}

void mod_main(HMODULE)
{
	//matdash::create_console();
	MH_Initialize();
	
	#define HOOK(addr, func) MH_CreateHook(\
		reinterpret_cast<void*>(base + addr),\
		reinterpret_cast<void*>(&func##H),\
		reinterpret_cast<void**>(&func##O)\
	)
	
	HOOK(0x170e50, LevelSettingsLayer_init);
	
	MH_EnableHook(MH_ALL_HOOKS);
	
	
	HOOK(LEVEL_EDITOR_LAYER__UPDATE, LevelEditorLayer_update); //this hook is enabled/disabled manually
}
