
#ifndef UNIFORM_H
#define UNIFORM_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Descriptor.h"

namespace Uniform {
namespace Default {

// PROJECTOR
namespace Projector {
struct DATA {
    glm::mat4 proj;
};
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData)
        : Buffer::Item(std::forward<const Buffer::Info>(info)),
          Descriptor::Base(UNIFORM::PROJECTOR_DEFAULT),
          Buffer::DataItem<DATA>(pData) {
        dirty = true;
    }
};
}  // namespace Projector

// FOG
namespace Fog {
typedef enum FLAG {
    FOG_LINEAR = 0x0000000,
    FOG_EXP = 0x00000001,
    FOG_EXP2 = 0x00000002,
    // THROUGH 0x0000000F (used by shader)
    BITS_MAX_ENUM = 0x7FFFFFFF
} FLAG;
struct DATA {
    float minDistance = 0.0f;
    float maxDistance = 40.0f;
    float density = 0.05f;
    FlagBits flags = FLAG::FOG_LINEAR;
    alignas(16) glm::vec3 color = CLEAR_COLOR;  // rem 4
};
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData)
        : Buffer::Item(std::forward<const Buffer::Info>(info)),
          Descriptor::Base(UNIFORM::FOG_DEFAULT),
          Buffer::DataItem<DATA>(pData) {
        dirty = true;
    }
};
}  // namespace Fog

}  // namespace Default
}  // namespace Uniform

namespace UniformDynamic {
namespace Matrix4 {
using DATA = glm::mat4;
struct CreateInfo : ::Buffer::CreateInfo {
    DATA data;
};
class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
        : Buffer::Item(std::forward<const Buffer::Info>(info)),
          Descriptor::Base(std::forward<const DESCRIPTOR>(UNIFORM_DYNAMIC::MATRIX_4)),
          Buffer::PerFramebufferDataItem<DATA>(pData) {
        data_ = pCreateInfo->data;
        setData();
    }
};
}  // namespace Matrix4
}  // namespace UniformDynamic

#endif  // !UNIFORM_H
