/*
    Physically-based Rendering Shader

    This comes from the cookbook, and is a physically-based reflection
    model. I will try to fill this out more once I can do more work on
    the math.
*/

#ifndef PBR_H
#define PBR_H

#include "Light.h"
#include "Object3d.h"
#include "Shader.h"
#include "Pipeline.h"
#include "Uniform.h"

// **********************
//      Light
// **********************

namespace Light {
namespace PBR {

class Positional : public Base {
   public:
    struct DATA {
        glm::vec3 position{};           // 12
        FlagBits flags{FLAG::SHOW};     // 4
        alignas(16) glm::vec3 L{0.6f};  // 12 light intensity
        // 4 rem
    };

    Positional(DATA data = {}) : Base(), data_(data){};

   private:
    inline VkDeviceSize getDataSize() const { return sizeof DATA; }
    inline const void *getData() { return &data_; }

    DATA data_;
};

}  // namespace PBR
}  // namespace Light

// **********************
//      Material
// **********************

namespace Material {
namespace PBR {

struct DATA {
    glm::vec3 color;  // diffCoeff
    FlagBits flags;
    float roughness;
    float __padding1;             // rem 4
    alignas(8) float __padding2;  // rem 8
};

static DATA getData(const Material::Info *pInfo) { return {pInfo->diffuseCoeff, pInfo->flags, pInfo->roughness}; }

}  // namespace PBR
}  // namespace Material

// **********************
//      Uniform
// **********************

namespace Uniform {
namespace PBR {

class LightPositional : public Base {
   public:
    LightPositional()
        : Base{UNIFORM::LIGHT_POSITIONAL_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(Light::PBR::Positional::DATA),
               "PBR Positional Light Unifrom Buffer"} {}
};

class Material : public Base {
   public:
    Material()
        : Base{UNIFORM::MATERIAL_DEFAULT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, sizeof(::Material::PBR::DATA),
               "PBR Material Dynamic Unifrom Buffer"} {}
};

}  // namespace PBR
}  // namespace Uniform

// **********************
//      Shader
// **********************

namespace Shader {
namespace PBR {

const std::string FRAG_FILENAME = "color.pbr.frag";
class ColorFrag : public Base {
   public:
    ColorFrag(const Shader::Handler &handler)
        : Base{handler,  //
               SHADER::PBR_COLOR_FRAG, FRAG_FILENAME, VK_SHADER_STAGE_FRAGMENT_BIT, "PBR Color Fragment Shader"} {}
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
    Color(const Pipeline::Handler &handler)
        : Base{handler,  //
               PIPELINE::PBR_COLOR,
               {SHADER::COLOR_VERT, SHADER::PBR_COLOR_FRAG},
               {PUSH_CONSTANT::PBR},
               VK_PIPELINE_BIND_POINT_GRAPHICS,
               "PBR Color"} {};

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

}  // namespace PBR
}  // namespace Pipeline

#endif  // !PBR_H