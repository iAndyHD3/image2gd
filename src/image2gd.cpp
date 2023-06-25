
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

void enableUpdateHook()
{
	MH_EnableHook(reinterpret_cast<void*>(gd::base + LEVEL_EDITOR_LAYER__UPDATE));
}

void disableUpdateHook()
{
	MH_DisableHook(reinterpret_cast<void*>(gd::base + LEVEL_EDITOR_LAYER__UPDATE));
}

bool isValidBitmap(const geometrize::Bitmap& bitmap)
{
	return bitmap.getWidth() != 0 && bitmap.getHeight() != 0;
}

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
	if(filePath.empty()) {
		return geometrize::Bitmap(0, 0, geometrize::rgba{0, 0, 0, 0});
	}
	
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
	
	//idk how to use win bitmap data
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
			bitmap.setPixel(x, image_h - y - 1, {topRed, topGreen, topBlue, topAlpha});

			std::uint8_t bottomRed = GetRValue(bottomPixel);
			std::uint8_t bottomGreen = GetGValue(bottomPixel);
			std::uint8_t bottomBlue = GetBValue(bottomPixel);
			std::uint8_t bottomAlpha = bottomPixel >> 24;
	
			bitmap.setPixel(x, y, {bottomRed, bottomGreen, bottomBlue, bottomAlpha});
		}
	}
	
	SelectObject(hMemDC, hOldBitmap);
	DeleteDC(hMemDC);
	ReleaseDC(nullptr, hdc);
	CloseClipboard();
	return bitmap;
	
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

float _stof(const std::string_view s) {
	float ret = -1.0f;
	std::from_chars(s.data(),s.data() + s.size(), ret);
	return ret;
}


constexpr const char* TOTAL_SHAPESPromt = 
	"The total shape count.\n"
	" For smaller images, recommended 100-300\n"
	" For bigger images, recommended > 500";
	
constexpr const char* stepShapeCountPromt =
	"Higher values will result in better quality but slower execution\n"
	" For smaller images, recommended > 500\n"
	" For bigger images, recommended < 500";

void addImage(geometrize::Bitmap bitmap, int totalShapes, int shapesPerStep)
{
	if(bitmap.getWidth() == 0 || bitmap.getHeight() == 0)
	{
		return;
	}
	
	geometrize::ImageRunnerOptions options;
	options.shapeCount = shapesPerStep;
	options.alpha = 255;
	options.maxShapeMutations = 50;
	options.shapeTypes = shapeTypesForNames("circle");
	
	geometrize::ImageRunner runner(bitmap);
	
	SHAPE_LOCK.lock();
	SHAPE_DATA.clear();
	SHAPE_LOCK.unlock();
	
	while(totalShapes > DRAWN_SHAPES)
	{
		if(!PROCESSING_IMAGE) //pause
		{
			break;
		}
		const std::vector<geometrize::ShapeResult> shapes{runner.step(options)};
		SHAPE_LOCK.lock();
		for(const auto& shape : shapes)
		{
			SHAPE_DATA.push_back(shape);
		}
		SHAPE_LOCK.unlock();
	}
	disableUpdateHook();
}