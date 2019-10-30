
#include "Instance.h"

void Instance::Obj3d::DATA::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(DATA);
    createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    // model
    auto modelOffset = offsetof(DATA, model);

    // column 0
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 0);
    // column 1
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 1);
    // column 2
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 2);
    // column 3
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 3);
}

Instance::Obj3d::Base::Base(const Buffer::Info&& info, Obj3d::DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<Obj3d::DATA>(pData),
      Instance::Base(STORAGE_BUFFER_DYNAMIC::DONT_CARE) {
    dirty = true;
}

void Instance::Obj3d::Base::putOnTop(const ::Obj3d::BoundingBoxMinMax& inBoundingBoxMinMax, uint32_t index) {
    if (index != MODEL_ALL) {
        ::Obj3d::AbstractBase::putOnTop(inBoundingBoxMinMax, std::forward<uint32_t>(index));
    } else {
        for (index = 0; index < BUFFER_INFO.count; index++) {
            auto myBoundingBoxMinMax = getBoundingBoxMinMax(true, index);
            auto t = TranslateToTop(inBoundingBoxMinMax, myBoundingBoxMinMax);
            transform(t, index);
        }
    }
}
