#ifndef TESSELLATION_H
#define TESSELLATION_H

#include <glm/glm.hpp>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Descriptor.h"
#include "Deferred.h"
#include "Pipeline.h"

namespace Shader {
struct CreateInfo;
namespace Tessellation {

extern const CreateInfo COLOR_VERT_CREATE_INFO;
extern const CreateInfo BEZIER_4_TESC_CREATE_INFO;
extern const CreateInfo BEZIER_4_TESE_CREATE_INFO;
extern const CreateInfo TRIANGLE_TESC_CREATE_INFO;
extern const CreateInfo TRIANGLE_TESE_CREATE_INFO;

}  // namespace Tessellation
}  // namespace Shader

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

    constexpr void decSegs() {
        if (pData_->outerLevel[Bezier::SEGMENTS] > 4) {
            pData_->outerLevel[Bezier::SEGMENTS]--;
            dirty = true;
        }
    }
    constexpr void incSegs() {
        pData_->outerLevel[Bezier::SEGMENTS]++;
        dirty = true;
    }
    constexpr void decInnerLevelTriangle() {
        if (pData_->innerLevel[0] > -1) {
            pData_->innerLevel[0]--;
            dirty = true;
        }
    }
    constexpr void incInnerLevelTriangle() {
        pData_->innerLevel[0]++;
        dirty = true;
    }
    constexpr void decOuterLevelTriangle() {
        for (auto i = 0; i < 3; i++) {
            if (pData_->outerLevel[i] > 0) {
                pData_->outerLevel[i]--;
                dirty = true;
            }
        }
    }
    constexpr void incOuterLevelTriangle() {
        for (auto i = 0; i < 3; i++) pData_->outerLevel[i]++;
        dirty = true;
    }
    constexpr void noTessTriangle() {
        for (auto i = 0; i < 3; i++) pData_->outerLevel[i] = 1;
        pData_->innerLevel[0] = 0;
        dirty = true;
    }
};
}  // namespace Default

}  // namespace Tessellation
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace Tessellation {

extern const CreateInfo DEFAULT_CREATE_INFO;

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
    void getTesselationInfoResources(CreateInfoResources& createInfoRes) override;
};

// BEZIER 4 (DEFERRED)
class Bezier4Deferred : public Base {
   public:
    Bezier4Deferred(Handler& handler);
};

// TRIANGLE (DEFERRED)
class TriangleDeferred : public Base {
   public:
    TriangleDeferred(Handler& handler);
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Tessellation
}  // namespace Pipeline

#endif  // !TESSELLATION_H