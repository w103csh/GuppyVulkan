/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Descriptor.h"

void Descriptor::Base::setDescriptorInfo(Set::ResourceInfo& info, const uint32_t index) const {
    assert(info.bufferInfos.size() && info.descCount);
    assert(info.uniqueDataSets > 0 && info.uniqueDataSets <= BUFFER_INFO.count);
    if (info.uniqueDataSets == 1 && info.descCount <= 1) {
        info.bufferInfos.at(0) = BUFFER_INFO.bufferInfo;
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