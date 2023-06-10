

#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>
#include <matdash/console.hpp>

#include <fmt/format.h>
#include <gd.h>

#include "image2gd.h"

#include "geometrize/shape/circle.h"
#include "geometrize/bitmap/rgba.h"
#include "geometrize/shaperesult.h"


#define MEMBERBYOFFSET(type, class, offset) *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(class) + offset)
#define MBO MEMBERBYOFFSET

using namespace gd;
using namespace cocos2d;



constexpr int CIRCLE_ID = 1764;



void addCircle(LevelEditorLayer* self, const geometrize::ShapeResult& result)
{
	
	geometrize::Circle* circle = dynamic_cast<geometrize::Circle*>(result.shape.get());
	
	geometrize::rgba color = result.color;
	
	float h,s,v;
	image2gd::RGBtoHSV(color.r, color.g, color.b, h, s, v);

	std::string hsv_string = fmt::format("{:.2}a{:.2}a{:.2}a1a1", h, s, v);
	
	const float x = circle->m_x * DRAW_SCALE;
	const float y = circle->m_y * DRAW_SCALE;
	const float scale = circle->m_r * DRAW_SCALE / 4;
	
	//https://wyliemaster.github.io/gddocs/#/resources/client/level-components/level-object?id=level-object
	std::string str = fmt::format(
		"1,{},2,{},3,{},32,{},41,1,42,1,43,{},44,{},25,{},21,{},20,{}",
		CIRCLE_ID, x, y, scale, hsv_string, hsv_string, Z_ORDER, 1010, Z_LAYER
	);
	//fmt::println("{}", str);
	
	self->addObjectFromString(str);
	Z_ORDER++;
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
	if(PROCESSING_IMAGE)
	{
		SHAPE_LOCK.lock();
		if(SHAPE_DATA.size() != 0)
		{
			for(geometrize::ShapeResult result : SHAPE_DATA)
			{
				addShape(self, result);
			}
			SHAPE_DATA.clear();
		}
		SHAPE_LOCK.unlock();
	}
	LevelEditorLayer_updateO(self, dt);
}

struct Callback
{
	void onImportImage(CCObject*)
	{
		if(!PROCESSING_IMAGE)
		{
			Z_ORDER = 0;
			std::thread(image2gd::addImage).detach();
		}
	}
};

bool (__thiscall* LevelSettingsLayer_initO)(CCLayer* self, void* settings, void* editor);
bool __fastcall LevelSettingsLayer_initH(CCLayer* self, void* edx, void* settings, void* editor)
{
	if(!LevelSettingsLayer_initO(self, settings, editor)) return false;
	if(!self) return true;
	
	//fmt::println("test");
	
	auto winSize = CCDirector::sharedDirector()->getWinSize() / 2;
	auto layer = (CCLayer*)self->getChildren()->objectAtIndex(0);
	if(!layer) return true;
	
	auto menu = (CCMenu*)layer->getChildren()->objectAtIndex(1);
	if(!menu) return true;
	
	auto spr = ButtonSprite::create("Import Image", 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 1);
	spr->setScale(0.65f);
	auto btn = CCMenuItemSpriteExtra::create(spr, self, menu_selector(Callback::onImportImage));
	btn->setPosition(menu->convertToNodeSpace({winSize.width - 100.0f, winSize.height - 130.0f}));
	menu->addChild(btn);
	return true;
	
}


void mod_main(HMODULE)
{
	matdash::create_console();
	MH_Initialize();
	
	//matdash breaks with update functions so im afraid to use it now
	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x1632b0),
		reinterpret_cast<void*>(&LevelEditorLayer_updateH),
		reinterpret_cast<void**>(&LevelEditorLayer_updateO) // note the &, this gets the address of the variable
	);
	
	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x170e50),
		reinterpret_cast<void*>(&LevelSettingsLayer_initH),
		reinterpret_cast<void**>(&LevelSettingsLayer_initO) // note the &, this gets the address of the variable
	);
	
	
	MH_EnableHook(MH_ALL_HOOKS);

}
