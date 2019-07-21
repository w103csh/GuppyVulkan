#ifndef STORAGE_H
#define STORAGE_H

#include "BufferItem.h"
#include "Descriptor.h"

namespace Storage {

namespace PostProcess {

// Offsets
constexpr uint8_t AccumulatedLuminace = 0;
constexpr uint8_t ImageSize = 1;

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