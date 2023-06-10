#include <vector>
#include <mutex>
#include "geometrize/shaperesult.h"

//important, inline and not static

inline bool PROCESSING_IMAGE = false;
inline std::vector<geometrize::ShapeResult> SHAPE_DATA;
inline std::mutex SHAPE_LOCK;
inline int SHAPES_DONE = 0;
inline int Z_ORDER = 0;
inline int DRAW_SCALE = 1;
inline int Z_LAYER = 0;

namespace image2gd
{

void addImage();
void RGBtoHSV(int r, int g, int b, float& h, float& s, float& v);


}