/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef TESSELLATION_H
#define TESSELLATION_H

#include <glm/glm.hpp>
#include <memory>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Descriptor.h"
#include "DescriptorManager.h"
#include "Deferred.h"
#include "Pipeline.h"

namespace Uniform {
namespace Tessellation {

namespace Bezier {
constexpr uint32_t SEGMENTS = 1;
constexpr uint32_t STRIPS = 0;
}  // namespace Bezier

namespace Default {
struct DATA {
    glm::ivec4 outerLevel;
    alignas(16) glm::ivec2 innerLevel;
    // rem 8
};
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData);

    virtual_inline void decSegs() {
        if (pData_->outerLevel[Bezier::SEGMENTS] > 4) {
            pData_->outerLevel[Bezier::SEGMENTS]--;
            dirty = true;
        }
    }
    virtual_inline void incSegs() {
        pData_->outerLevel[Bezier::SEGMENTS]++;
        dirty = true;
    }
    virtual_inline void decInnerLevelTriangle() {
        if (pData_->innerLevel[0] > -1) {
            pData_->innerLevel[0]--;
            dirty = true;
        }
    }
    virtual_inline void incInnerLevelTriangle() {
        pData_->innerLevel[0]++;
        dirty = true;
    }
    virtual_inline void decOuterLevelTriangle() {
        for (auto i = 0; i < 3; i++) {
            if (pData_->outerLevel[i] > 0) {
                pData_->outerLevel[i]--;
                dirty = true;
            }
        }
    }
    virtual_inline void incOuterLevelTriangle() {
        for (auto i = 0; i < 3; i++) pData_->outerLevel[i]++;
        dirty = true;
    }
    virtual_inline void noTessTriangle() {
        for (auto i = 0; i < 3; i++) pData_->outerLevel[i] = 1;
        pData_->innerLevel[0] = 0;
        dirty = true;
    }
};
}  // namespace Default

}  // namespace Tessellation
}  // namespace Uniform

namespace UniformDynamic {
namespace Tessellation {
namespace Phong {
struct DATA {
    glm::vec4 data0 = {
        1.0f,   // [0] maxLevel (adaptive)
        0.75f,  // [1] alpha (adaptive)
        1.0f,   // [2] innerLevel
        1.0f    // [3] outerLevel
    };
};
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData);
};
using Manager = Descriptor::Manager<Descriptor::Base, Base, std::shared_ptr>;
}  // namespace Phong
}  // namespace Tessellation
}  // namespace UniformDynamic

namespace Descriptor {
namespace Set {
namespace Tessellation {
extern const CreateInfo DEFAULT_CREATE_INFO;
extern const CreateInfo PHONG_CREATE_INFO;
}  // namespace Tessellation
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {
struct CreateInfo;
class Handler;
namespace Tessellation {

class Base : public Graphics {
   public:
    const bool IS_DEFERRED;
    const uint32_t PATCH_CONTROL_POINTS;

   protected:
    Base(Handler& handler, const CreateInfo* pCreateInfo, const uint32_t patchControlPoints, const bool isDeferred = true);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getShaderStageInfoResources(CreateInfoResources& createInfoRes) override;
    void getTessellationInfoResources(CreateInfoResources& createInfoRes) override;
};

// BEZIER 4 (DEFERRED)
class Bezier4Deferred : public Base {
   public:
    Bezier4Deferred(Handler& handler);
};

// PHONG TRIANGLE COLOR (DEFERRED)
class PhongTriColorDeferred : public Base {
   public:
    PhongTriColorDeferred(Handler& handler);

   protected:
    PhongTriColorDeferred(Pipeline::Handler& handler, const CreateInfo* pCreateInfo) : Base(handler, pCreateInfo, 3) {}
};

// PHONG TRIANGLE COLOR WIREFRAME (DEFERRED)
std::unique_ptr<Pipeline::Base> MakePhongTriColorWireframeDeferred(Pipeline::Handler& handler);
class PhongTriColorWireframeDeferred : public PhongTriColorDeferred {
   public:
    PhongTriColorWireframeDeferred(Handler& handler, const CreateInfo* pCreateInfo);
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Tessellation
}  // namespace Pipeline

#endif  // !TESSELLATION_H