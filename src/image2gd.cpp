
#include "image2gd.h"

#include "portable-file-dialogs.h"
#include "stb_image.h"
#include "stb_image_write.h"

#include "geometrize/shaperesult.h"
#include "geometrize/bitmap/bitmap.h"
#include "geometrize/bitmap/rgba.h"
#include "geometrize/runner/imagerunner.h"
#include "geometrize/runner/imagerunneroptions.h"
#include "geometrize/shape/circle.h"

#include <fmt/format.h>
#include <charconv>


void RGBtoHSV(int r, int g, int b, float& h, float& s, float& v)
{
	auto _max = [](auto a, auto b) -> auto {
		return a > b ? a : b;
	};

	auto _min = [](auto a, auto b) -> auto {
		return a < b ? a : b;
	};

	// Normalize RGB values to the range of 0 to 1
	float r_normalized = r / 255.0;
	float g_normalized = g / 255.0;
	float b_normalized = b / 255.0;

	// Calculate value (maximum of RGB values)
	v = _max(_max(r_normalized, g_normalized), b_normalized);

	// Calculate saturation
	if (v == 0)
		s = 0;
	else
		s = (v - _min(_min(r_normalized, g_normalized), b_normalized)) / v;

	// Calculate hue
	if (s == 0)
		h = 0;
	else
	{
		float delta = v - _min(_min(r_normalized, g_normalized), b_normalized);
		if (v == r_normalized)
			h = 60 * (g_normalized - b_normalized) / delta;
		else if (v == g_normalized)
			h = 60 * (2 + (b_normalized - r_normalized) / delta);
		else if (v == b_normalized)
			h = 60 * (4 + (r_normalized - g_normalized) / delta);

		if (h < 0)
			h += 360;
	}
}

extern "C" char *InputBox(char *Prompt, char *Title = (char *)"", char *Default = (char *)"");
std::string winInputBox(std::string_view promt, std::string_view title, std::string_view _default)
{

	return { InputBox((char*)promt.data(), (char*)title.data(), (char*)_default.data()) };
}

geometrize::Bitmap readImage(const std::string& filePath)
{
	const char* path{filePath.c_str()};
	int w = 0;
	int h = 0;
	stbi_set_flip_vertically_on_load(true);
	std::uint8_t* dataPtr{stbi_load(path, &w, &h, nullptr, 4)};
	if(dataPtr == nullptr) {
		return geometrize::Bitmap(0, 0, geometrize::rgba{0, 0, 0, 0});
	}
	const std::vector<std::uint8_t> data{dataPtr, dataPtr + (w * h * 4)};
	delete dataPtr;

	const geometrize::Bitmap bitmap(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), data);
	return bitmap;
}

// Helper function to convert a string into the Geometrize shape types that the given string names
// e.g. "circle rotated_rectangle" returns the ShapeTypes for those two types of shape (bitwise-or'd together into a single ShapeTypes instance)
geometrize::ShapeTypes shapeTypesForNames(const std::string& str)
{
	// Split string into words based on whitespace
	std::istringstream iss(str);
	const std::vector<std::string> shapeNames(std::istream_iterator<std::string>{iss},
									std::istream_iterator<std::string>());
	
	std::vector<geometrize::ShapeTypes> shapeTypes;
	
	// Convert the shape names into ShapeTypes
	for(const std::string& shapeName : shapeNames) {
		for(const std::pair<geometrize::ShapeTypes, std::string>& p : geometrize::shapeTypeNames) {
			if(p.second == shapeName) {
				shapeTypes.push_back(p.first);
			}
		}
	}
	
	if(shapeTypes.empty()) {
		std::cout << "Bad shape names provided, defaulting to ellipses \n";
		return geometrize::ELLIPSE;
	}
	
	// Combine shape types together
	std::underlying_type<geometrize::ShapeTypes>::type combinedShapeTypes = 0;
	for (const auto& shapeType : shapeTypes) {
		combinedShapeTypes |= shapeType;
	}
	return geometrize::ShapeTypes(combinedShapeTypes);
}



geometrize::Bitmap readImageFromClipboard()
{
	if (!OpenClipboard(nullptr))
	{
		std::cout << "Failed to open clipboard." << std::endl;
		return geometrize::Bitmap(0, 0, geometrize::rgba{0, 0, 0, 0});
	}

	HBITMAP hBitmap = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
	if (!hBitmap)
	{
		std::cout << "No image found on the clipboard." << std::endl;
		CloseClipboard();
		return geometrize::Bitmap(0, 0, geometrize::rgba{0, 0, 0, 0});
	}

	BITMAP win_bitmap;
	if (!GetObject(hBitmap, sizeof(BITMAP), &win_bitmap))
	{
		std::cout << "Failed to retrieve bitmap information." << std::endl;
		CloseClipboard();
		return geometrize::Bitmap(0, 0, geometrize::rgba{0, 0, 0, 0});
	}

	HDC hdc = GetDC(nullptr);
	HDC hMemDC = CreateCompatibleDC(hdc);
	HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hMemDC, hBitmap));

	std::uint32_t image_w = static_cast<std::uint32_t>(win_bitmap.bmWidth);
	std::uint32_t image_h = static_cast<std::uint32_t>(win_bitmap.bmHeight);
	
	geometrize::Bitmap bitmap(image_w, image_h, geometrize::rgba{0, 0, 0, 0});

	for (std::uint32_t y = 0; y < image_h / 2; ++y)
	{
		for (std::uint32_t x = 0; x < image_w; ++x)
		{
			COLORREF topPixel = GetPixel(hMemDC, x, y);
			COLORREF bottomPixel = GetPixel(hMemDC, x, image_h - y - 1);
	
			std::uint8_t topRed = GetRValue(topPixel);
			std::uint8_t topGreen = GetGValue(topPixel);
			std::uint8_t topBlue = GetBValue(topPixel);
			std::uint8_t topAlpha = topPixel >> 24;
	
			std::uint8_t bottomRed = GetRValue(bottomPixel);
			std::uint8_t bottomGreen = GetGValue(bottomPixel);
			std::uint8_t bottomBlue = GetBValue(bottomPixel);
			std::uint8_t bottomAlpha = bottomPixel >> 24;
	
			bitmap.setPixel(x, y, {bottomRed, bottomGreen, bottomBlue, bottomAlpha});
			bitmap.setPixel(x, image_h - y - 1, {topRed, topGreen, topBlue, topAlpha});
		}
	}
	
	SelectObject(hMemDC, hOldBitmap);
	DeleteDC(hMemDC);
	ReleaseDC(nullptr, hdc);
	CloseClipboard();
	return bitmap;
}



const char* getImagePathDialog()
{
	auto f = pfd::open_file("Image to draw", pfd::path::home(), { "Image files(.png .jpg)", "*.png *.jpg", }, false).result();
	//fmt::println("vec: {}", fmt::join(f, ", "));
	
	if(f.empty()) return nullptr;
	
	if(const auto& str = f[0]; !str.empty()) {
		return str.c_str();
	}
	return nullptr;
}


void logShapeResult(std::size_t step, const geometrize::ShapeResult& result)
{
	switch(result.shape->getType())
	{
		case geometrize::ShapeTypes::CIRCLE:
		{
			geometrize::rgba color = result.color;
			auto circle = dynamic_cast<geometrize::Circle*>(result.shape.get());
			fmt::println("STEP: {} | Circle | x: {}, y: {}, r: {}, RGBA: {},{},{},{}",
			step, circle->m_x, circle->m_y, circle->m_r, color.r, color.g, color.b, color.a);
		}
	}
}

int _stoi(const std::string_view s) {
	int ret = -1;
	std::from_chars(s.data(),s.data() + s.size(), ret);
	return ret;
}

void updateLabel(bool enabled)
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
	
	if(enabled)
	{
		float percentage;
		if(STEP >= TOTAL_SHAPES) {
			percentage = 100.0f;
		}
		else {
			percentage = ((static_cast<float>(STEP) / static_cast<float>(TOTAL_SHAPES)) * 100);
		}
		std::string labelstr = fmt::format("{}/{} | {:.2}%", STEP, TOTAL_SHAPES, percentage);
		label->setString(labelstr.c_str());
		label->setVisible(true);
	}
	else
	{
		label->setVisible(false);
	}
}

constexpr const char* TOTAL_SHAPESPromt = 
	"The total shape count.\n"
	" For smaller images, recommended 100-300\n"
	" For bigger images, recommended > 500";
	
constexpr const char* stepShapeCountPromt =
	"Higher values will result in better quality but slower execution\n"
	" For smaller images, recommended > 500\n"
	" For bigger images, recommended < 500";

void addImage()
{
	SHAPE_LOCK.lock();
	SHAPE_DATA.clear();
	SHAPE_LOCK.unlock();
	
	//const char* inputImagePath = getImagePathDialog();
	//if(!inputImagePath) return;
	
	//fmt::println("path: {}", inputImagePath);
	//const geometrize::Bitmap bitmap = readImage(inputImagePath);
	const geometrize::Bitmap bitmap = readImageFromClipboard();
	if(bitmap.getWidth() == 0 || bitmap.getHeight() == 0)
	{
		//std::cout << "Failed to read input image from: " << inputImagePath;
		return;
	}
	
	//returns false if the user cancelled
	auto handle_int_input = [](int& n, std::string promt, std::string title, std::string _default, int min = 1, int max = 2147483648) -> bool
	{
		do
		{
			std::string userInput = winInputBox(promt, title, _default);
			if(userInput.empty()) return false;	
			n = _stoi(userInput);
		}
		while(min > n && n > max);
		return true;
	};
	
	if(!handle_int_input(TOTAL_SHAPES, TOTAL_SHAPESPromt, "Total Shape Count", std::to_string(bitmap.getWidth() + bitmap.getHeight()))) {
		return;
	}
	
	int stepShapeCount = 0;
	if(!handle_int_input(stepShapeCount, stepShapeCountPromt, "Step Shape Count", "300")) {
		return;
	}
	
	if(!handle_int_input(DRAW_SCALE, "The gd object scale, 1 for default", "Scale", "1")) {
		return;
	}
	
	if(!handle_int_input(Z_LAYER, "The gd z layer where objects will be created", "Z Layer", "0", 0)) {
		return;
	}
	
	//fmt::println("total: {}, stepCount: {}", TOTAL_SHAPES, stepShapeCount);
	
	// the options for geometrizing the image
	geometrize::ImageRunnerOptions options;
	options.shapeCount = stepShapeCount;
	options.alpha = 255;
	options.shapeTypes = shapeTypesForNames("circle");
	
	geometrize::ImageRunner runner(bitmap);
	STEP = 0;
	updateLabel(true);
	
	PROCESSING_IMAGE = true;
	if(BTN_SPR) { 
		BTN_SPR->setString("Stop current");
	}
	
	int total = TOTAL_SHAPES + 2;

	for(STEP = 1; STEP < total; STEP++)
	{
		if(!PROCESSING_IMAGE || !gd::LevelEditorLayer::get()) {
			if(BTN_SPR) {
				BTN_SPR->setString("Import Image");
			}
			break;
		}
		
		const std::vector<geometrize::ShapeResult> shapes{runner.step(options)};
		for(const auto& result : shapes)
		{
			geometrize::rgba color = result.color;
			
			//switch(result.shape->getType())
			//{
			//	case geometrize::ShapeTypes::CIRCLE:
			//	{
			//		logShapeResult(STEP, result);
			//	}
			//}
			SHAPE_LOCK.lock();
			std::copy(shapes.begin(), shapes.end(), std::back_inserter(SHAPE_DATA));
			SHAPE_LOCK.unlock();
		}
	}
	updateLabel(false);
	PROCESSING_IMAGE = false;
	pfd::message("image2gd", "Image finished processing", pfd::choice::ok, pfd::icon::info);
}