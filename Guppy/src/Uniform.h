
#ifndef UNIFORM_H
#define UNIFORM_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Descriptor.h"
#include "Helpers.h"

namespace Uniform {

class Base : public virtual Buffer::Item, public Descriptor::Interface {
   public:
    // DESCRIPTOR
    inline void setWriteInfo(VkWriteDescriptorSet& write, uint32_t index = 0) const override {
        write.pBufferInfo = &BUFFER_INFO.bufferInfo;
    }

   protected:
    Base() {}
};

namespace Default {

// PROJECTOR
namespace Projector {
struct DATA {
    glm::mat4 proj;
};
class Base : public Uniform::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData)
        : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
          Uniform::Base(),                                       //
          Buffer::DataItem<DATA>(pData)                          //
    {
        DIRTY = true;
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
class Base : public Uniform::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData)
        : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
          Uniform::Base(),                                       //
          Buffer::DataItem<DATA>(pData)                          //
    {
        DIRTY = true;
    }
};
}  // namespace Fog

}  // namespace Default

}  // namespace Uniform

#endif  // !UNIFORM_H