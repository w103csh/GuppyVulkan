#ifndef TESSELLATION_H
#define TESSELLATION_H

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Descriptor.h"
#include "Pipeline.h"

namespace Shader {
struct CreateInfo;
namespace Tessellation {

extern const CreateInfo COLOR_VERT_CREATE_INFO;
extern const CreateInfo BEZIER_4_TESC_CREATE_INFO;
extern const CreateInfo BEZIER_4_TESE_CREATE_INFO;

}  // namespace Tessellation
}  // namespace Shader

namespace Uniform {
namespace Tessellation {
namespace Bezier {

struct DATA {
    int strips;
    int segments;
};

class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData);

    constexpr void decSegs() {
        if (pData_->segments > 4) {
            pData_->segments--;
            dirty = true;
        }
    }
    constexpr void incSegs() {
        pData_->segments++;
        dirty = true;
    }
};

}  // namespace Bezier
}  // namespace Tessellation
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace Tessellation {

extern const CreateInfo BEZIER_CREATE_INFO;

}  // namespace Tessellation
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {
struct CreateInfo;
class Handler;
namespace Tessellation {

// BEZIER 4 (DEFERRED)
class DeferredBezier4 : public Graphics {
   public:
    DeferredBezier4(Handler& handler);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getTesselationInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Tessellation
}  // namespace Pipeline

#endif  // !TESSELLATION_H