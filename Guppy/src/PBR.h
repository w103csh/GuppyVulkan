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
namespace Positional {
struct DATA {
    glm::vec3 position{};           // 12 (camera space)
    FlagBits flags = FLAG::SHOW;    // 4
    alignas(16) glm::vec3 L{0.6f};  // 12 light intensity
    // 4 rem
};
class Base : public Light::Base<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, CreateInfo *pCreateInfo);

    void update(glm::vec3 &&position);

    inline const glm::vec3 &getPosition() { return position; }

    void transform(const glm::mat4 t) override {
        Object3d::transform(t);
        position = getWorldSpacePosition();
    }

   private:
    glm::vec3 position;  // (world space)
};
}  // namespace Positional
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
    // 16
    alignas(8) float roughness;
    // 8 (4 rem)
};

struct CreateInfo : public Material::CreateInfo {
    float roughness;
};

class Base : public Buffer::DataItem<DATA>, public Material::Base {
   public:
    Base(const Buffer::Info &&info, DATA *pData, CreateInfo *pCreateInfo);

    FlagBits getFlags() override { return pData_->flags; }
    void setFlags(FlagBits flags) override {
        pData_->flags = flags;
        setData();
    }

    void setTextureData() override;
    void setTinyobjData(const tinyobj::material_t &m) override;

   private:
    inline void setData() override { DIRTY = true; };
};

}  // namespace PBR
}  // namespace Material

// **********************
//      Shader
// **********************

namespace Shader {
namespace PBR {

class ColorFrag : public Base {
   public:
    ColorFrag(Shader::Handler &handler);
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
    glm::mat4 model;
    Material::PBR::DATA material;
};

class Color : public Pipeline::Base {
   public:
    Color(Pipeline::Handler &handler);
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

}  // namespace PBR
}  // namespace Pipeline

#endif  // !PBR_H