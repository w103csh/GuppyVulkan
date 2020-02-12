/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef SHADOW_H
#define SHADOW_H

#include <array>
#include <glm/glm.hpp>
#include <string_view>
#include <vulkan/vulkan.hpp>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Light.h"
#include "Pipeline.h"

namespace Texture {
struct CreateInfo;
namespace Shadow {

const std::string_view MAP_2D_ARRAY_ID = "Shadow 2D Array Map Texture";
extern const CreateInfo MAP_2D_ARRAY_CREATE_INFO;

const std::string_view MAP_CUBE_ARRAY_ID = "Shadow Map Cube Array Texture";

const std::string_view OFFSET_2D_ID = "Shadow Offset 2D Texture";
CreateInfo MakeOffsetTex();

}  // namespace Shadow
}  // namespace Texture

namespace Uniform {
namespace Shadow {

struct DATA {
    // 0: width
    // 1: height
    // 2: depth
    // 3: radius
    glm::vec4 data{0.0f};
};

class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData);
};

}  // namespace Shadow
}  // namespace Uniform

namespace Light {
namespace Shadow {

// POSITIONAL (This never worked right...)
namespace Positional {

struct CreateInfo : public Buffer::CreateInfo {
    glm::mat4 proj{1.0};
    glm::mat4 mainCameraSpaceToWorldSpace{1.0};
};

struct DATA {
    glm::mat4 proj;
};

class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    void update(const glm::mat4 &m, const uint32_t frameIndex = UINT32_MAX);

   private:
    glm::mat4 proj_;
};

}  // namespace Positional

// CUBE
namespace Cube {

constexpr uint32_t LAYERS = 6;

struct CreateInfo : public Buffer::CreateInfo {
    glm::vec3 position{0.0f, 0.0f, 0.0f};  // (world space)
    float n = 0.1f;                        // near distance
    float f = 1000.0f;                     // far distance
    glm::vec3 La{0.2f};                    // Ambient intensity
    glm::vec3 L{0.6f};                     // Diffuse and specular intensity
    FlagBits flags = FLAG::SHOW;
};

struct DATA {
    std::array<glm::mat4, LAYERS> viewProjs;  // cube map mvps
    glm::vec4 data0;                          // xyz: worldPosition, w: cutoff
    glm::vec3 cameraPosition;                 //
    FlagBits flags;                           //
    alignas(16) glm::vec3 La;                 // Ambient intensity
    alignas(16) glm::vec3 L;                  // Diffuse and specular intensity
};

class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    void setPosition(const glm::vec3 position, const uint32_t frameIndex = UINT32_MAX);
    auto getPosition() const { return glm::vec3(data_.data0); }

    void update(glm::vec3 &&position, const uint32_t frameIndex = UINT32_MAX);

   private:
    void setViews();

    float near_;
    /**
     * Look direction:
     * 0:  X
     * 1: -X
     * 2:  Y
     * 3: -Y
     * 4:  Z
     * 5: -Z
     */
    std::array<glm::mat4, LAYERS> views_;
    glm::mat4 proj_;
};

}  // namespace Cube

}  // namespace Shadow
}  // namespace Light

namespace Descriptor {
namespace Set {
namespace Shadow {

extern const CreateInfo CUBE_UNIFORM_ONLY_CREATE_INFO;
extern const CreateInfo CUBE_ALL_CREATE_INFO;
extern const CreateInfo SAMPLER_CREATE_INFO;
extern const CreateInfo SAMPLER_OFFSET_CREATE_INFO;

}  // namespace Shadow
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
// struct CreateInfo;
namespace Shadow {

const CreateInfo COLOR_VERT_CREATE_INFO = {
    SHADER::SHADOW_COLOR_VERT,
    "Shadow Color Vertex Shader",
    "vert.color.shadow.glsl",
    vk::ShaderStageFlagBits::eVertex,
};
const CreateInfo TEX_VERT_CREATE_INFO = {
    SHADER::SHADOW_TEX_VERT,
    "Shadow Texture Vertex Shader",
    "vert.texture.shadow.glsl",
    vk::ShaderStageFlagBits::eVertex,
};
const CreateInfo CUBE_GEOM_CREATE_INFO = {
    SHADER::SHADOW_CUBE_GEOM,
    "Shadow Cube Geometry Shader",
    "geom.shadow.cube.glsl",
    vk::ShaderStageFlagBits::eGeometry,
};
const CreateInfo FRAG_CREATE_INFO = {
    SHADER::SHADOW_FRAG,
    "Shadow Fragment Shader",
    "frag.shadow.glsl",
    vk::ShaderStageFlagBits::eFragment,
};

}  // namespace Shadow
}  // namespace Shader

namespace Pipeline {

struct CreateInfo;
class Handler;

void GetShadowRasterizationStateInfoResources(Pipeline::CreateInfoResources &createInfoRes);

namespace Shadow {

class Color : public Pipeline::Graphics {
   public:
    Color(Handler &handler);

   private:
    void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes) override;
};

class Texture : public Pipeline::Graphics {
   public:
    Texture(Handler &handler);

   private:
    void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes) override;
};

}  // namespace Shadow
}  // namespace Pipeline

#endif  // !SHADOW_H
