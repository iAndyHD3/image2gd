#include <vector>
#include <mutex>
#include <string>
#include <string_view>
#include <atomic>
#include "MinHook.h"

#include <gd.h>

#include "geometrize/shaperesult.h"
#include "geometrize/bitmap/bitmap.h"
#include "geometrize/shape/shapetypes.h"

//important, inline and not static


inline std::mutex SHAPE_LOCK;
inline std::vector<geometrize::ShapeResult> SHAPE_DATA;

 
inline gd::ButtonSprite* BTN_SPR = nullptr;
inline float DRAW_SCALE = 1.0f;
inline int Z_LAYER = 0;
inline int TOTAL_SHAPES = 0;
inline std::atomic<int> DRAWN_SHAPES = 0;
inline std::atomic<bool> PROCESSING_IMAGE = false;

#define LEVEL_EDITOR_LAYER__UPDATE 0x1632b0 //constexpr doesnt work
constexpr int CIRCLE_ID = 1764;


void addImage(geometrize::Bitmap bitmap, int totalShapes, int shapesPerStep);
void RGBtoHSV(int r, int g, int b, float& h, float& s, float& v);
void logShapeResult(std::size_t step, const geometrize::ShapeResult& result);

std::string winInputBox(std::string_view promt, std::string_view title, std::string_view _default);
float _stof(const std::string_view s);
int _stoi(const std::string_view s);
bool isValidBitmap(const geometrize::Bitmap& bitmap);
const char* getImagePathDialog();
geometrize::Bitmap readImageFromClipboard();
geometrize::Bitmap readImage(const std::string& filePath);

geometrize::ShapeTypes shapeTypesForNames(const std::string& str);

void enableUpdateHook();
void disableUpdateHook();
