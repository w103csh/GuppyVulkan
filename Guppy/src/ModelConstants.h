#ifndef MODEL_CONSTANTS_H
#define MODEL_CONSTANTS_H

#include <string>

namespace Model {

using index = uint32_t;
constexpr auto INDEX_MAX = UINT32_MAX;

struct Settings {
    std::string modelPath = "";
    bool faceVertexColorsRGB = false;
    bool reverseFaceWinding = false;
    bool smoothNormals = false;
    bool doVisualHelper = false;
    float visualHelperLineSize = 0.1f;
};

}  // namespace Model

#endif  // !MODEL_CONSTANTS_H