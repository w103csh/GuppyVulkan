/*
    Physically-based Rendering Shader

    This comes from the cookbook, and is a physically-based reflection
    model. I will try to fill this out more once I can do more work on
    the math.
*/

#ifndef PBR_H
#define PBR_H

#include "DescriptorSet.h"
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
    glm::vec3 position{};            // 12 (camera space)
    FlagBits flags = FLAG::SHOW;     // 4
    alignas(16) glm::vec3 L{30.0f};  // 12 light intensity
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

    void setIntensity(glm::vec3 L) { pData_->L = L; }

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

struct DATA : public Material::DATA {
    alignas(16) float roughness;
    // rem (12)
};

struct CreateInfo : public Material::CreateInfo {
    float roughness = 0.43f;
};

class Base : public Buffer::DataItem<DATA>, public Material::Base {
   public:
    Base(const Buffer::Info &&info, PBR::DATA *pData, PBR::CreateInfo *pCreateInfo);

    FlagBits getFlags() override { return pData_->flags; }
    void setFlags(FlagBits flags) override {
        pData_->flags = flags;
        setData();
    }

    void setTextureData() override;
    void setTinyobjData(const tinyobj::material_t &m) override;

    void setRoughness(float r);

   private:
    inline void setData() override { DIRTY = true; };
};

}  // namespace PBR
}  // namespace Material

// **********************
//      Descriptor Set
// **********************

namespace Descriptor {
namespace Set {
namespace PBR {
class Uniform : public Set::Base {
   public:
    Uniform();
};
}  // namespace PBR
}  // namespace Set
}  // namespace Descriptor

// **********************
//      Shader
// **********************

namespace Shader {

namespace PBR {
class ColorFragment : public Base {
   public:
    ColorFragment(Shader::Handler &handler);
};
class TextureFragment : public Base {
   public:
    TextureFragment(Shader::Handler &handler);
};
}  // namespace PBR

namespace Link {
namespace PBR {
class Fragment : public Shader::Link::Base {
   public:
    Fragment(Shader::Handler &handler);
};
}  // namespace PBR
}  // namespace Link

}  // namespace Shader

// **********************
//      Pipeline
// **********************

namespace Pipeline {

struct CreateInfoResources;

namespace PBR {

class Color : public Pipeline::Base {
   public:
    Color(Pipeline::Handler &handler);
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
};

class Texture : public Pipeline::Base {
   public:
    Texture(Pipeline::Handler &handler);
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
};

}  // namespace PBR
}  // namespace Pipeline

#endif  // !PBR_H