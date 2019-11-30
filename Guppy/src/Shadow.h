/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef SHADOW_H
#define SHADOW_H

#include <glm/glm.hpp>
#include <string_view>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Pipeline.h"

namespace Texture {
struct CreateInfo;
namespace Shadow {

const std::string_view MAP_2D_ARRAY_ID = "Shadow 2D Array Map Texture";
extern const CreateInfo MAP_2D_ARRAY_CREATE_INFO;

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
    Base(const Buffer::Info&& info, DATA* pData);
};

}  // namespace Shadow
}  // namespace Uniform

namespace Light {
namespace Shadow {

struct CreateInfo : public Buffer::CreateInfo {
    glm::mat4 proj{1.0};
    glm::mat4 mainCameraSpaceToWorldSpace{1.0};
};

struct DATA {
    glm::mat4 proj;
};

class Positional : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Positional(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);

    void update(const glm::mat4& m, const uint32_t frameIndex = UINT32_MAX);

   private:
    glm::mat4 proj_;
};

}  // namespace Shadow
}  // namespace Light

namespace Descriptor {
namespace Set {
namespace Shadow {

extern const CreateInfo UNIFORM_CREATE_INFO;
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
    VK_SHADER_STAGE_VERTEX_BIT,
};
const CreateInfo TEX_VERT_CREATE_INFO = {
    SHADER::SHADOW_TEX_VERT,
    "Shadow Texture Vertex Shader",
    "vert.texture.shadow.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};
const CreateInfo FRAG_CREATE_INFO = {
    SHADER::SHADOW_FRAG,
    "Shadow Fragment Shader",
    "frag.shadow.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
};

}  // namespace Shadow
}  // namespace Shader

namespace Pipeline {

struct CreateInfo;
class Handler;

void GetShadowRasterizationStateInfoResources(Pipeline::CreateInfoResources& createInfoRes);

namespace Shadow {

class Color : public Pipeline::Graphics {
   public:
    Color(Handler& handler);

   private:
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

class Texture : public Pipeline::Graphics {
   public:
    Texture(Handler& handler);

   private:
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Shadow
}  // namespace Pipeline

#endif  // !SHADOW_H
