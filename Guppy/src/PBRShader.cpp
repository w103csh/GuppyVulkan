
#include <string>

#include "PBRShader.h"

using namespace Shader;

const std::string PBR_COLOR_FRAG_FILENAME = "color.pbr.frag";

PBRColor::PBRColor(const VkDevice &dev)
    : Base(SHADER_TYPE::PBR_COLOR_FRAG, PBR_COLOR_FRAG_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, "PBR Color Fragment Shader") {
}
