#include <vector>
#include <mutex>
#include "geometrize/shaperesult.h"
#include <gd.h>

//important, inline and not static

inline bool PROCESSING_IMAGE = false;
inline std::vector<geometrize::ShapeResult> SHAPE_DATA;
inline std::mutex SHAPE_LOCK;
inline int SHAPES_DONE = 0;
inline int DRAW_SCALE = 1;
inline int Z_LAYER = 0;
inline int TOTAL_SHAPES = 0;
inline int STEP = 0;

inline gd::ButtonSprite* BTN_SPR = nullptr;

constexpr int CIRCLE_ID = 1764;

namespace image2gd
{
	void addImage();
	void RGBtoHSV(int r, int g, int b, float& h, float& s, float& v);
	void updateLabel(bool enabled = true);
}