
#include "Descriptor.h"

void Descriptor::Base::setDescriptorInfo(Set::ResourceInfo& info, const uint32_t index) const {
    assert(info.uniqueDataSets > 0 && info.uniqueDataSets <= BUFFER_INFO.count);
    if (info.uniqueDataSets == 1 && info.descCount <= 1) {
        if (info.bufferInfos.empty()) {
            info.bufferInfos.push_back(BUFFER_INFO.bufferInfo);
            info.descCount++;
        } else {
            info.bufferInfos.at(0) = BUFFER_INFO.bufferInfo;
        }
    } else {
        for (uint32_t i = 0; i < info.uniqueDataSets; i++) {
            auto offset = index + (i * info.descCount);
            assert(offset < info.bufferInfos.size());
            // Add a buffer info per number of unique data sets needed.
            info.bufferInfos.at(offset) = getBufferInfo(i);
        }
    }
}

VkDescriptorBufferInfo Descriptor::Base::getBufferInfo(const uint32_t index) const {
    auto bufferInfo = BUFFER_INFO.bufferInfo;
    bufferInfo.offset += index * bufferInfo.range;
    return bufferInfo;
}