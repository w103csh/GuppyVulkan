#ifndef STORAGE_H
#define STORAGE_H

#include "BufferItem.h"
#include "Descriptor.h"

namespace Storage {

namespace ScreenSpace {

// Offsets
constexpr uint8_t AccumulatedLuminace = 0;
constexpr uint8_t ImageSize = 1;
constexpr uint8_t LogAverageLuminance = 0;

struct DATA {
    // [0] accumulated luminance
    // [1] image size
    glm::uvec4 uData1{0};
    // [0] log average luminance for HDR
    glm::vec4 fData1{0.0f};
};

class PostProcess : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    PostProcess(const Buffer::Info&& info, DATA* pData, const Buffer::CreateInfo* pCreateInfo)
        : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
          Buffer::PerFramebufferDataItem<DATA>(pData) {}

    inline void setImageSize(uint32_t&& imageSize) {
        data_.uData1[ImageSize] = imageSize;
        setData();
    }

    inline void reset(const uint32_t frameIndex) {
        data_.uData1[AccumulatedLuminace] = 0;
        setData(frameIndex);
    }
};

}  // namespace ScreenSpace

}  // namespace Storage

#endif  // !STORAGE_H