#ifndef INSTANCE_H
#define INSTANCE_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "BufferItem.h"
#include "Obj3d.h"
#include "Types.h"

// clang-format off
namespace Obj3d { class Instance; }
// clang-format on

namespace Instance {

// BASE

class Base : public virtual Buffer::Item {
   protected:
    Base() {}
};

template <typename TDATA, typename TBase>
struct CreateInfo : public Buffer::CreateInfo {
    std::vector<TDATA> data;
    std::shared_ptr<TBase> pSharedData = nullptr;
};

// OBJ3D

namespace Obj3d {

const uint32_t MODEL_ALL = UINT32_MAX;

struct DATA {
    static void getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes);
    glm::mat4 model{1.0f};
};

class Base;
struct CreateInfo : public Instance::CreateInfo<DATA, Base> {};

class Base : public ::Obj3d::AbstractBase, public Buffer::DataItem<DATA>, public Instance::Base {
    friend class ::Obj3d::Instance;

   public:
    Base(const Buffer::Info&& info, DATA* pData);

    void putOnTop(const ::Obj3d::BoundingBoxMinMax& inBoundingBoxMinMax, const uint32_t index = MODEL_ALL) override;

    inline void transform(const glm::mat4 t, const uint32_t index = 0) override {
        ::Obj3d::AbstractBase::transform(std::forward<const glm::mat4>(t), std::forward<const uint32_t>(index));
        dirty = true;
    }

    inline const glm::mat4& model(const uint32_t index = 0) const override { return (pData_ + index)->model; }
};

}  // namespace Obj3d

}  // namespace Instance

#endif  // !INSTANCE_H