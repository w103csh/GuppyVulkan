/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * 
 * Physically-based Rendering Shader
 */

#ifndef PBR_H
#define PBR_H

#include "ConstantsAll.h"
#include "DescriptorSet.h"
#include "Light.h"
#include "Obj3d.h"
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
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    void update(glm::vec3 &&position, const uint32_t frameIndex);

    inline const glm::vec3 &getPosition() { return position; }

    void transform(const glm::mat4 t, const uint32_t index = 0) override {
        Obj3d::AbstractBase::transform(t);
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

class Base : public Material::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, PBR::DATA *pData, const PBR::CreateInfo *pCreateInfo);

    FlagBits getFlags() override { return pData_->flags; }
    void setFlags(FlagBits flags) override {
        pData_->flags = flags;
        setData();
    }

    void setTextureData() override { status_ = SetDefaultTextureData(this, pData_); }
    void setTinyobjData(const tinyobj::material_t &m) override;

    void setRoughness(float r);
};

}  // namespace PBR
}  // namespace Material

// **********************
//      Descriptor Set
// **********************

namespace Descriptor {
namespace Set {
namespace PBR {
extern const CreateInfo UNIFORM_CREATE_INFO;
}  // namespace PBR
}  // namespace Set
}  // namespace Descriptor

// **********************
//      Shader
// **********************

namespace Shader {
namespace PBR {
extern const CreateInfo COLOR_FRAG_CREATE_INFO;
extern const CreateInfo TEX_FRAG_CREATE_INFO;
}  // namespace PBR

namespace Link {
namespace PBR {
extern const CreateInfo FRAG_CREATE_INFO;
extern const CreateInfo MATERIAL_CREATE_INFO;
}  // namespace PBR
}  // namespace Link

}  // namespace Shader

// **********************
//      Pipeline
// **********************

namespace Pipeline {
namespace PBR {

class Color : public Pipeline::Graphics {
   public:
    Color(Pipeline::Handler &handler);
};

class Texture : public Pipeline::Graphics {
   public:
    Texture(Pipeline::Handler &handler);
};

}  // namespace PBR
}  // namespace Pipeline

#endif  // !PBR_H