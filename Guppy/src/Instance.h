#ifndef INSTANCE_H
#define INSTANCE_H

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "BufferItem.h"
#include "Object3d.h"

// clang-format off
namespace Mesh { class Base; }
// clang-format on

namespace Instance {

const uint32_t BINDING = 1;

class Base : public Object3d, public virtual Buffer::Item {
    // TODO: if this stored anywhere but Mesh::Base remove this
    // except for maybe Model::Base.
    friend class Mesh::Base;

   protected:
    Base() {}
};

template <typename TDATA>
struct CreateInfo : public Buffer::CreateInfo {
    std::vector<TDATA> data;
};

// **********************
//      Default
// **********************

namespace Default {

struct DATA {
    static void getBindingDescriptions(std::vector<VkVertexInputBindingDescription>& descs);
    static void getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& descs);
    glm::mat4 model{1.0f};
};

struct CreateInfo : public Instance::CreateInfo<DATA> {};

class Base : public Buffer::DataItem<Default::DATA>, public Instance::Base {
   public:
    Base(const Buffer::Info&& info, Default::DATA* pData);

    inline void transform(const glm::mat4 t, uint32_t index = 0) override {
        Object3d::transform(std::forward<const glm::mat4>(t), std::forward<uint32_t>(index));
        DIRTY = true;
    }

   private:
    inline const glm::mat4& model(uint32_t index = 0) const override { return (pData_ + index)->model; }
};

}  // namespace Default

}  // namespace Instance

#endif  // !INSTANCE_H