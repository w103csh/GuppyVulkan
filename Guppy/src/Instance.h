/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef INSTANCE_H
#define INSTANCE_H

#include <algorithm>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include <Common/Types.h>

#include "BufferItem.h"
#include "Descriptor.h"
#include "Obj3d.h"

// clang-format off
namespace Obj3d { class Instance; }
// clang-format on

namespace Instance {

// BASE

class Base : public virtual Buffer::Item {
   public:
    // The concept of active count is a quick and dirty way to get a dynamic number of instances for draws. I don't feel like
    // doing anything more complicated atm.
    constexpr auto getActiveCount() const { return activeCount_; }
    constexpr void setActiveCount(uint32_t count) {
        assert(count < BUFFER_INFO.count);
        activeCount_ = (std::min)(count, BUFFER_INFO.count);
    }

   protected:
    Base() : activeCount_(BUFFER_INFO.count) {}

    uint32_t activeCount_;
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

    inline const glm::mat4& getModel(const uint32_t index = 0) const override { return (pData_ + index)->model; }

    inline void transform(const glm::mat4 t, const uint32_t index = 0) override {
        ::Obj3d::AbstractBase::transform(std::forward<const glm::mat4>(t), std::forward<const uint32_t>(index));
        dirty = true;
    }
    void putOnTop(const ::Obj3d::BoundingBoxMinMax& inBoundingBoxMinMax, const uint32_t index = MODEL_ALL) override;
    inline void setModel(const glm::mat4 m, const uint32_t index = 0) override {
        ::Obj3d::AbstractBase::setModel(std::forward<const glm::mat4>(m), std::forward<const uint32_t>(index));
        dirty = true;
    }

   protected:
    inline glm::mat4& model(const uint32_t index = 0) override { return (pData_ + index)->model; }
};

}  // namespace Obj3d

}  // namespace Instance

#endif  // !INSTANCE_H