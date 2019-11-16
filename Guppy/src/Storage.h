#ifndef STORAGE_H
#define STORAGE_H

#include <glm/glm.hpp>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Descriptor.h"

namespace Storage {

// VECTOR4
namespace Vector4 {

void GetInputDescriptions(Pipeline::CreateInfoResources& createInfoRes, const VkVertexInputRate&& inputRate);

class Base;
struct CreateInfo : Buffer::CreateInfo {
    CreateInfo(const glm::uvec3 localSize) : localSize(localSize) {
        countInRange = true;
        dataCount = localSize.x * localSize.y * localSize.z;
    }
    TYPE type = TYPE::DONT_CARE;
    STORAGE_BUFFER_DYNAMIC descType = STORAGE_BUFFER_DYNAMIC::DONT_CARE;
    glm::uvec3 localSize;
};
class Base : public Buffer::DataItem<DATA>, public Descriptor::Base {
   public:
    const TYPE VECTOR_TYPE;
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);

    void set(const DATA& v, const VkDeviceSize index) { pData_[index] = v; }
};

}  // namespace Vector4

namespace PostProcess {

struct DATA {
    DATA();
    // [0] accumulated luminance
    // [1] image size
    glm::uvec4 uData0{0};
    float weights[5];
    float edgeThreshold;
    alignas(8) float logAverageLuminance;
    // rem 4
};

class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const Buffer::CreateInfo* pCreateInfo);

    inline void setImageSize(uint32_t&& imageSize) {
        data_.uData0[ImageSize] = imageSize;
        setData();
    }

    inline void reset(const uint32_t frameIndex) {
        data_.uData0[AccumulatedLuminace] = 0;
        setData(frameIndex);
    }
};

}  // namespace PostProcess

}  // namespace Storage

#endif  // !STORAGE_H