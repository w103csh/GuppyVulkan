/*
    Physically-based Rendering Shader

    This comes from the cookbook, and is a physically-based reflection
    model. I will try to fill this out more once I can do more work on
    the math.
*/

#ifndef PBR_H
#define PBR_H

#include "Object3d.h"
#include "Shader.h"
#include "Pipeline.h"

namespace Material {
namespace PBR {

struct DATA {
    glm::vec3 color;  // diffCoeff
    FlagBits flags;
    float roughness;
    float __padding1;             // rem 4
    alignas(8) float __padding2;  // rem 8
};

static Material::PBR::DATA getData(const std::unique_ptr<Material::Base> &pMaterial) {
    return {pMaterial->getDiffuseCoeff(), pMaterial->getFlags(), pMaterial->getRoughness()};
}

}  // namespace PBR
}  // namespace Material

// **********************
//      Shader
// **********************

namespace Shader {
namespace PBR {

const std::string FRAG_FILENAME = "color.pbr.frag";
class ColorFrag : public Base {
   public:
    ColorFrag()
        : Base{                         //
               SHADER::PBR_COLOR_FRAG,  //
               FRAG_FILENAME,
               VK_SHADER_STAGE_FRAGMENT_BIT,
               "PBR Color Fragment Shader",
               {},
               {/*SHADER_TYPE::UTIL_FRAG*/}} {}
};

}  // namespace PBR
}  // namespace Shader

// **********************
//      Pipeline
// **********************

namespace Pipeline {

struct CreateInfoResources;

namespace PBR {

struct PushConstant {
    Object3d::DATA obj3d;
    Material::PBR::DATA material;
};

class Color : public Pipeline::Base {
   public:
    Color()
        : Base{PIPELINE::PBR_COLOR,
               {SHADER::COLOR_VERT, SHADER::PBR_COLOR_FRAG},
               {PUSH_CONSTANT::DEFAULT},
               VK_PIPELINE_BIND_POINT_GRAPHICS,
               "PBR Color"} {};

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

}  // namespace PBR
}  // namespace Pipeline

#endif  // !PBR_H