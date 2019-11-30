/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <glm/glm.hpp>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Pipeline.h"

namespace Shader {
namespace Geometry {
extern const CreateInfo WIREFRAME_CREATE_INFO;
extern const CreateInfo SILHOUETTE_CREATE_INFO;
}  // namespace Geometry
namespace Link {
namespace Geometry {
extern const CreateInfo WIREFRAME_CREATE_INFO;
}  // namespace Geometry
}  // namespace Link
}  // namespace Shader

namespace Uniform {
namespace Geometry {

namespace Default {
struct DATA {
    glm::mat4 viewport{1.0f};
    glm::vec4 color{0.05f, 0.0f, 0.05f, 1.0f};
    glm::vec4 data1{
        1.0f,    // line width
        0.25f,   // silhouette pctExtend
        0.015f,  // silhouette edgeWidth
        0.0f     // rem
    };
};  // namespace Default
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData);

    void updateLine(const glm::vec4 color, const float width);
    void updateViewport(const VkExtent2D& extent);
};
}  // namespace Default

}  // namespace Geometry
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace Geometry {

extern const CreateInfo DEFAULT_CREATE_INFO;

}  // namespace Geometry
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {
struct CreateInfo;
class Handler;
namespace Geometry {

class Silhouette : public Graphics {
   public:
    const bool IS_DEFERRED;
    Silhouette(Handler& handler, const bool isDeferred = true);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Geometry
}  // namespace Pipeline

#endif  //! GEOMETRY_H
