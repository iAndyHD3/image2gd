#include <vector>
#include <mutex>
#include <string>
#include <string_view>

#include <gd.h>

#include "geometrize/shaperesult.h"
#include "geometrize/bitmap/bitmap.h"
#include "geometrize/shape/shapetypes.h"



//important, inline and not static

inline bool PROCESSING_IMAGE = false;
inline std::vector<geometrize::ShapeResult> SHAPE_DATA;
inline int SHAPES_DONE = 0;
inline int DRAW_SCALE = 1;
inline int Z_LAYER = 0;
inline std::mutex SHAPE_LOCK;
inline int TOTAL_SHAPES = 0;
inline int STEP = 0;

inline gd::ButtonSprite* BTN_SPR = nullptr;

constexpr int CIRCLE_ID = 1764;


void addImage();
void RGBtoHSV(int r, int g, int b, float& h, float& s, float& v);
void updateLabel(bool enabled = true);

std::string winInputBox(std::string_view promt, std::string_view title, std::string_view _default);
void logShapeResult(std::size_t step, const geometrize::ShapeResult& result);
int _stoi(const std::string_view s);

const char* getImagePathDialog();
geometrize::Bitmap readImageFromClipboard();
geometrize::Bitmap readImage(const std::string& filePath);

geometrize::ShapeTypes shapeTypesForNames(const std::string& str);

